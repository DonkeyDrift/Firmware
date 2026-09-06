"""Drift/Judge 设置 NVS 单次写回归测试。

行车中在 Drift/Judge 设置页点保存时，HTTP handler 在唯一主循环里同步写 NVS；
旧实现逐键 put（drift 12 键 / judge 10 键），每次 put 都触发一次 nvs_commit 写
flash，控制输出在同一循环被推迟数十~数百 ms、车会瞬时停顿。修复后：

- 保存路径必须是单次 putBytes 定长 blob（含格式版本字段，static_assert 钉住尺寸）；
- load 必须先读 blob，读不到（长度/版本不匹配）再回退旧逐键格式——老车已调好
  的参数无损保留，且只有值真的来自旧键时才顺手迁移写成 blob。
"""
import pathlib
import re

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
WIFI_MANAGER_CPP = PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiManager.cpp"

DRIFT_FIELDS = [
    "steeringGyroSign",
    "maxYawRate",
    "kp",
    "kd",
    "maxSteeringCorrection",
    "gyroFilterAlpha",
    "spinThreshold",
    "steeringThreshold",
    "continuousThrottle",
    "pulseThrottle",
    "pulseFreqHz",
    "pulseDuty",
]
DRIFT_LEGACY_KEYS = [
    "MUS4_PREF_DRIFT_STEERING_GYRO_SIGN_KEY",
    "MUS4_PREF_DRIFT_MAX_YAW_RATE_KEY",
    "MUS4_PREF_DRIFT_KP_KEY",
    "MUS4_PREF_DRIFT_KD_KEY",
    "MUS4_PREF_DRIFT_MAX_STEERING_CORRECTION_KEY",
    "MUS4_PREF_DRIFT_GYRO_FILTER_ALPHA_KEY",
    "MUS4_PREF_DRIFT_SPIN_THRESHOLD_KEY",
    "MUS4_PREF_DRIFT_STEERING_THRESHOLD_KEY",
    "MUS4_PREF_DRIFT_CONTINUOUS_THROTTLE_KEY",
    "MUS4_PREF_DRIFT_PULSE_THROTTLE_KEY",
    "MUS4_PREF_DRIFT_PULSE_FREQ_HZ_KEY",
    "MUS4_PREF_DRIFT_PULSE_DUTY_KEY",
]
JUDGE_FIELDS = [
    "collisionThreshold",
    "bigTurnThreshold",
    "windowSize",
    "collisionPenalty",
    "turnSmoothnessWeight",
    "rangeMatchWeight",
    "gyroStabilityWeight",
    "bigTurnStabilityWeight",
    "speedStabilityWeight",
    "throttleStabilityWeight",
]
JUDGE_LEGACY_KEYS = [
    "MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY",
    "MUS4_PREF_JUDGE_BIG_TURN_THRESHOLD_KEY",
    "MUS4_PREF_JUDGE_WINDOW_SIZE_KEY",
    "MUS4_PREF_JUDGE_COLLISION_PENALTY_KEY",
    "MUS4_PREF_JUDGE_TURN_SMOOTHNESS_WEIGHT_KEY",
    "MUS4_PREF_JUDGE_RANGE_MATCH_WEIGHT_KEY",
    "MUS4_PREF_JUDGE_GYRO_STABILITY_WEIGHT_KEY",
    "MUS4_PREF_JUDGE_BIG_TURN_STABILITY_WEIGHT_KEY",
    "MUS4_PREF_JUDGE_SPEED_STABILITY_WEIGHT_KEY",
    "MUS4_PREF_JUDGE_THROTTLE_STABILITY_WEIGHT_KEY",
]


def _source():
    return WIFI_MANAGER_CPP.read_text(encoding="utf-8")


def _function_body(source, signature):
    m = re.search(re.escape(signature) + r"\s*\{(.*?)^\}", source, re.DOTALL | re.MULTILINE)
    assert m, f"未找到函数体: {signature}"
    return m.group(1)


def _struct_body(source, name):
    m = re.search(r"struct\s+" + name + r"\s*\{(.*?)\};", source, re.DOTALL)
    assert m, f"未找到结构体: {name}"
    return m.group(1)


def test_drift_save_uses_single_putbytes_blob():
    """drift 保存路径：单次 putBytes blob，一次 nvs_commit 写 flash。"""
    body = _function_body(_source(), "bool saveDriftConfigPreference(const DriftConfig& config)")
    assert "putBytes(" in body, "drift 保存应使用 putBytes 单次写 blob"
    assert "MUS4_PREF_DRIFT_CFG_BLOB_KEY" in body
    assert body.count("mus4Prefs.put") == 1, "drift 保存只允许一次 NVS put（一次 nvs_commit）"
    assert "putFloat" not in body and "putInt(" not in body, "drift 保存不应再逐键 put"
    assert "memset(&blob, 0, sizeof(blob))" in body, "blob 应 memset 清零（含预留/填充字节）"
    assert "blob.version = MUS4_CFG_BLOB_VERSION" in body, "blob 应写入格式版本字段"
    for field in DRIFT_FIELDS:
        assert f"blob.{field} = config.{field};" in body, f"drift blob 漏了字段 {field}"
    assert "written != sizeof(blob)" in body, "putBytes 返回值应与 blob 尺寸校验"


def test_judge_save_uses_single_putbytes_blob():
    """judge 保存路径：单次 putBytes blob，一次 nvs_commit 写 flash。"""
    body = _function_body(_source(), "bool saveJudgeConfigPreference(const JudgeConfig& config)")
    assert "putBytes(" in body, "judge 保存应使用 putBytes 单次写 blob"
    assert "MUS4_PREF_JUDGE_CFG_BLOB_KEY" in body
    assert body.count("mus4Prefs.put") == 1, "judge 保存只允许一次 NVS put（一次 nvs_commit）"
    assert "putFloat" not in body and "putUChar" not in body, "judge 保存不应再逐键 put"
    assert "memset(&blob, 0, sizeof(blob))" in body, "blob 应 memset 清零（含预留/填充字节）"
    assert "blob.version = MUS4_CFG_BLOB_VERSION" in body, "blob 应写入格式版本字段"
    for field in JUDGE_FIELDS:
        assert f"blob.{field} = config.{field};" in body, f"judge blob 漏了字段 {field}"
    assert "written != sizeof(blob)" in body, "putBytes 返回值应与 blob 尺寸校验"


def test_blob_structs_have_version_field_and_static_size_assert():
    """blob 结构体：定长布局、版本字段、static_assert 钉住尺寸，供将来扩展。"""
    source = _source()
    drift_struct = _struct_body(source, "DriftConfigBlob")
    judge_struct = _struct_body(source, "JudgeConfigBlob")
    for struct_body, name in ((drift_struct, "DriftConfigBlob"), (judge_struct, "JudgeConfigBlob")):
        assert "uint8_t version;" in struct_body, f"{name} 应含格式版本字段"
        assert "uint8_t reserved[2];" in struct_body, f"{name} 应含对齐预留字节"
    assert "static_assert(sizeof(DriftConfigBlob) ==" in source, "DriftConfigBlob 应 static_assert 尺寸"
    assert "static_assert(sizeof(JudgeConfigBlob) ==" in source, "JudgeConfigBlob 应 static_assert 尺寸"
    assert "static const uint8_t MUS4_CFG_BLOB_VERSION = 1;" in source
    # 字段清单与旧逐键保存一一对应，逐一核对别漏
    assert "int8_t steeringGyroSign;" in drift_struct
    assert len(re.findall(r"\bfloat \w+;", drift_struct)) == 11, "DriftConfigBlob 应有 11 个 float 字段"
    assert "uint8_t windowSize;" in judge_struct
    assert len(re.findall(r"\bfloat \w+;", judge_struct)) == 9, "JudgeConfigBlob 应有 9 个 float 字段"
    for field in DRIFT_FIELDS:
        assert re.search(r"\b" + field + r"\b", drift_struct), f"DriftConfigBlob 漏了字段 {field}"
    for field in JUDGE_FIELDS:
        assert re.search(r"\b" + field + r"\b", judge_struct), f"JudgeConfigBlob 漏了字段 {field}"


def test_drift_load_prefers_blob_and_keeps_legacy_key_fallback():
    """drift load：先读 blob（长度+版本校验），读不到回退旧逐键读取。"""
    body = _function_body(_source(), "void loadDriftConfigPreference()")
    assert "getBytesLength(MUS4_PREF_DRIFT_CFG_BLOB_KEY)" in body, "load 应先按长度探测 blob"
    assert "getBytes(MUS4_PREF_DRIFT_CFG_BLOB_KEY" in body
    assert "blob.version == MUS4_CFG_BLOB_VERSION" in body, "blob 版本不匹配应回退旧键"
    for field in DRIFT_FIELDS:
        assert f"config.{field} = blob.{field};" in body, f"blob 路径漏了字段 {field}"
    # 旧逐键回退路径必须保留：老车已调好的参数无损保留
    for key in DRIFT_LEGACY_KEYS:
        assert key in body, f"drift load 回退路径漏了旧键 {key}"
    assert "mus4Prefs.getFloat(" in body and "mus4Prefs.getInt(" in body
    # 只有值真的来自旧键才顺手迁移写成 blob，全新设备不在 load 里多写 flash
    assert "mus4Prefs.isKey(MUS4_PREF_DRIFT_STEERING_GYRO_SIGN_KEY)" in body
    assert "saveDriftConfigPreference(config);" in body


def test_judge_load_prefers_blob_and_keeps_legacy_key_fallback():
    """judge load：先读 blob（长度+版本校验），读不到回退旧逐键读取。"""
    body = _function_body(_source(), "void loadJudgeConfigPreference()")
    assert "getBytesLength(MUS4_PREF_JUDGE_CFG_BLOB_KEY)" in body, "load 应先按长度探测 blob"
    assert "getBytes(MUS4_PREF_JUDGE_CFG_BLOB_KEY" in body
    assert "blob.version == MUS4_CFG_BLOB_VERSION" in body, "blob 版本不匹配应回退旧键"
    for field in JUDGE_FIELDS:
        assert f"config.{field} = blob.{field};" in body, f"blob 路径漏了字段 {field}"
    # 旧逐键回退路径必须保留：老车已调好的参数无损保留
    for key in JUDGE_LEGACY_KEYS:
        assert key in body, f"judge load 回退路径漏了旧键 {key}"
    assert "mus4Prefs.getFloat(" in body and "mus4Prefs.getUChar(" in body
    # 只有值真的来自旧键才顺手迁移写成 blob，全新设备不在 load 里多写 flash
    assert "mus4Prefs.isKey(MUS4_PREF_JUDGE_COLLISION_THRESHOLD_KEY)" in body
    assert "saveJudgeConfigPreference(config);" in body
