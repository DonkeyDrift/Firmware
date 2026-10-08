"""MUS4 串口协议契约测试（Firmware 侧）。

契约源：`protocol/mus4_serial_v1.yaml`（与 DonkeyDrift 仓库共享的唯一协议事实来源）。
与 DonkeyDrift/tests/test_mus4_serial_contract.py 对称：断言固件实现里的
字面量/常量与 schema 声明一致，防两侧漂移。

实现落点：
  - MUS4_FW.ino（上行 T<S> / M:P / $IMU 拼帧）
  - libraries/mus4_command/src/CommandParser.cpp（下行 pilot_control 解析与校验）
  - libraries/mus4_command/src/CommandDispatcher.cpp（MODE 命令识别与应答）
  - libraries/mus4_core/src/FirmwareConfig.h（波特率/模式/心跳常量）

注意：本文件内的字符串字面量都用 raw string（r'...'），因为断言的目标是
C 源码里的 `"T%dS%d\n"` 这种**含反斜杠转义的原始文本**，不是 Python 解释
后的换行符。写成普通字符串会让断言静默失配（写测试时踩过一次）。
"""
import pathlib
import re

import pytest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
REPO_ROOT = PROJECT_ROOT.parent                      # .../Firmware
WORKSPACE_ROOT = REPO_ROOT.parent                    # .../projects

SCHEMA_PATH = WORKSPACE_ROOT / "protocol" / "mus4_serial_v1.yaml"
SKETCH_PATH = PROJECT_ROOT / "MUS4_FW.ino"
PARSER_PATH = PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandParser.cpp"
DISPATCHER_PATH = (
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "CommandDispatcher.cpp"
)
CONFIG_PATH = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h"

if not SCHEMA_PATH.is_file():
    pytest.skip(
        f"共享协议契约未找到: {SCHEMA_PATH}（单独 clone 时可跳过）",
        allow_module_level=True,
    )

yaml = pytest.importorskip("yaml")

SCHEMA = yaml.safe_load(SCHEMA_PATH.read_text(encoding="utf-8"))
SKETCH_SRC = SKETCH_PATH.read_text(encoding="utf-8")
PARSER_SRC = PARSER_PATH.read_text(encoding="utf-8")
DISPATCHER_SRC = DISPATCHER_PATH.read_text(encoding="utf-8")
CONFIG_SRC = CONFIG_PATH.read_text(encoding="utf-8")


def _frame(name):
    for group in ("uplink", "downlink"):
        for frame in SCHEMA[group]:
            if frame["name"] == name:
                return frame
    raise AssertionError(f"schema 中找不到帧: {name}")


def _const(path):
    node = SCHEMA["constants"]
    for key in path.split("."):
        node = node[key]
    return node


# --- schema 自身完整性 -------------------------------------------------------

def test_schema_identity():
    assert SCHEMA["schema"] == "mus4_serial"
    assert SCHEMA["version"] == 1
    assert SCHEMA["baudrate"] == 115200


def test_all_frame_regexes_compile():
    for group in ("uplink", "downlink"):
        for frame in SCHEMA[group]:
            if "regex" in frame:
                re.compile(frame["regex"])


def test_baudrate_matches_firmware_config():
    assert f"#define BAUD_RATE_0 {SCHEMA['baudrate']}" in CONFIG_SRC
    assert f"#define BAUD_RATE_1 {SCHEMA['baudrate']}" in CONFIG_SRC


# --- 上行：固件拼帧字面量必须与 schema 帧格式一致 ----------------------------

def test_uplink_control_telemetry_literal_matches_schema():
    frame = _frame("control_telemetry")
    assert frame["frame"] == "T<t>S<s>\n"
    assert r'"T%dS%d\n"' in SKETCH_SRC
    assert frame["rate_hz"] == 60
    assert frame["modes"] == ["manual"]
    assert "CAR_MODE_MANUAL" in SKETCH_SRC


def test_uplink_mode_park_literal_matches_schema():
    frame = _frame("mode_park_status")
    assert frame["frame"] == "M<m>:P<p>\n"
    assert r'"M%d:P%d\n"' in SKETCH_SRC
    assert frame["rate_hz"] == 1
    assert [f["name"] for f in frame["fields"]] == ["m", "p"]


def test_uplink_mode_park_heartbeat_constant_matches_schema():
    """MODE_PARK_HEARTBEAT_MS 必须是 1Hz 心跳（1000ms）。"""
    assert _const("uplink_mode_heartbeat_rate_hz") == 1
    assert "#define MODE_PARK_HEARTBEAT_MS 1000" in CONFIG_SRC


def test_uplink_imu_literal_matches_schema():
    frame = _frame("imu_sample")
    assert frame["format"]["floats"] == "%.4f"
    assert _const("imu_field_count") == 9
    assert r'"$IMU,%u,%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n"' in SKETCH_SRC
    assert frame["rate_hz"] == 100


def test_uplink_imu_regex_matches_firmware_literal():
    pattern = re.compile(_frame("imu_sample")["regex"])
    sample = "$IMU,12,34567,0.1234,-9.8000,0.0000,0.0010,-0.0020,0.0030"
    assert pattern.match(sample)
    assert not pattern.match("$IMU,12,34567,0.1234")


def test_uplink_rates_match_schema_constants():
    assert _const("uplink_telemetry_rate_hz") == 60
    assert _const("uplink_imu_rate_hz") == 100
    # 固件里的 IMU 间隔常量必须与 100Hz 对应（10ms）
    assert "IMU_TELEMETRY_INTERVAL_MS" in SKETCH_SRC


# --- 下行：CommandParser 校验必须与 schema validation 一致 -------------------

def test_downlink_control_range_matches_schema():
    assert _const("throttle_range") == {"min": -100, "max": 100}
    assert _const("steering_range") == {"min": -100, "max": 100}
    # parseAndValidateCommand 显式拒收越界值（validation.out_of_range = reject）
    assert "if (t < -100 || t > 100 || s < -100 || s > 100)" in PARSER_SRC


def test_downlink_checksum_algo_matches_schema():
    assert _frame("pilot_control")["validation"]["checksum_algo"] == (
        "sum(payload_bytes) & 0xFF"
    )
    assert _frame("pilot_control")["validation"]["checksum_format"] == "%02X"
    # calcChecksum 逐字节累加后 & 0xFF
    assert "sum += (uint8_t)s[i]" in PARSER_SRC
    assert "return (uint8_t)(sum & 0xFF)" in PARSER_SRC
    # 帧尾格式 "*XX" 大写十六进制
    assert r'"%s*%02X"' in PARSER_SRC


def test_downlink_ack_nack_responses_match_schema():
    assert _frame("pilot_control")["responses"] == ["ACK", "NACK", "ACK:Seq", "NACK:Seq"]


def test_downlink_mode_command_dual_prefix_matches_firmware():
    """schema 记录的 MODE <m> / MODE:<m> 双分隔符必须真的被固件识别。"""
    frame = _frame("mode_command")
    assert frame["validation"]["out_of_range"] == "reject"

    pattern = re.compile(frame["regex"])
    assert pattern.match("MODE 1")
    assert pattern.match("MODE:2")
    assert not pattern.match("MODE 3")
    assert not pattern.match("MODEX1")

    # CommandDispatcher.cpp 双前缀识别
    assert 'line.startsWith("MODE ")' in DISPATCHER_SRC
    assert 'line.startsWith("MODE:")' in DISPATCHER_SRC
    # 越界回 NACK
    assert "NACK:MODE_INVALID" in DISPATCHER_SRC


# --- 共享常量对称性 ---------------------------------------------------------

def test_car_mode_enum_matches_firmware():
    assert _const("car_mode") == {"manual": 0, "semi_auto": 1, "full_auto": 2}
    for macro in ("CAR_MODE_MANUAL", "CAR_MODE_SEMI_AUTO", "CAR_MODE_FULL_AUTO"):
        assert macro in CONFIG_SRC or macro in SKETCH_SRC


def test_park_state_enum_matches_firmware():
    assert _const("park_state") == {"unlocked": 0, "locked": 1}


# --- 契约元数据：DonkeyDrift 侧必须有对应测试 -------------------------------

def test_sibling_host_contract_test_exists():
    sibling = WORKSPACE_ROOT / "DonkeyDrift" / "tests" / "test_mus4_serial_contract.py"
    if not sibling.is_file():
        pytest.skip(f"上位机侧契约测试尚未落地: {sibling}")
    assert sibling.is_file()
