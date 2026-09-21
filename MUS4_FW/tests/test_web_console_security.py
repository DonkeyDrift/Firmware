import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
WEB_SERVER_CPP = (
    PROJECT_ROOT / "libraries" / "mus4_web" / "src" / "WebConsoleServer.cpp"
).read_text(encoding="utf-8")
WIRELESS_CONSOLE_CPP = (
    PROJECT_ROOT / "libraries" / "mus4_command" / "src" / "WirelessConsole.cpp"
).read_text(encoding="utf-8")
WIFI_OTA_CPP = (
    PROJECT_ROOT / "libraries" / "mus4_wifi" / "src" / "WifiOta.cpp"
).read_text(encoding="utf-8")
WIFI_CONSOLE_TYPES_H = (
    PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "WifiConsoleTypes.h"
).read_text(encoding="utf-8")


def _function_body(source, signature_pattern):
    match = re.search(signature_pattern + r"\s*\{(?P<body>.*?)\n\}", source, re.DOTALL)
    assert match, f"未找到函数：{signature_pattern}"
    return match.group("body")


# --- POST /api/devmode 鉴权门禁 ---

def test_devmode_set_requires_authentication():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebDevModeSet\(\)")
    assert "isWirelessConsoleAuthDisabled()" in body
    assert 'wifiWebServer.send(403, "application/json", "{\\"error\\":\\"auth_required\\"}")' in body
    # 门禁必须先于保存动作
    assert body.index("auth_required") < body.index("saveDevModePreference")


# --- /api/cmd?target=serial 直转 Serial2 门禁 ---

def test_serial_target_command_requires_authentication():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebCommand\(\)")
    assert 'target.equalsIgnoreCase("serial")' in body
    assert "isWirelessConsoleAuthDisabled()" in body
    # 门禁必须出现在转发 Serial2 与写 web 日志之前
    assert body.index("auth_required") < body.index("Serial2.print")


# --- 空 POST /update 不得重启 ---

def test_update_post_requires_started_upload_before_restart():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebUpdatePost\(\)")
    assert "s_wifiWebUpdateStarted" in body
    # 未发生鉴权通过的上传时返回 400/NACK，绝不进入 ACK + ESP.restart()
    assert "NACK:NO_UPLOAD" in body
    assert body.index("s_wifiWebUpdateStarted") < body.index("ESP.restart()")


def test_update_abort_happens_after_auth_check():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebUpdateUpload\(\)")
    # Update.abort() 必须在鉴权检查之后，避免未认证请求干扰进行中的 OTA
    assert body.index("isWifiWebUpdateAuthOk()") < body.index("Update.abort()")
    # 鉴权通过才置位"上传已开始"标志
    assert body.index("isWifiWebUpdateAuthOk()") < body.index("s_wifiWebUpdateStarted = true")


# --- OTA 上传 abort / 空闲超时兜底 ---

def test_upload_handler_cleans_up_on_aborted():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebUpdateUpload\(\)")
    aborted = body[body.index("UPLOAD_FILE_ABORTED"):]
    # core 3.3.10 会把 UPLOAD_FILE_ABORTED 送达 upload handler，abort 后必须在
    # 这里直接清理（POST handler 不会执行）
    assert "resetOtaAfterFailedUpload()" in aborted
    assert "closeWifiOtaWindow(" in aborted


def test_upload_handler_tracks_activity_timestamp():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebUpdateUpload\(\)")
    assert "s_lastOtaActivityMs = millis()" in body
    write_branch = body[body.index("UPLOAD_FILE_WRITE"):]
    assert "s_lastOtaActivityMs = millis()" in write_branch


def test_ota_idle_timeout_backstop_in_update_wifi_ota():
    body = _function_body(WIFI_OTA_CPP, r"void updateWifiOta\(OtaRuntimeState& os, WifiRuntimeState& ws\)")
    assert "WIFI_OTA_IDLE_TIMEOUT_MS" in body
    assert "wifiWebOtaLastActivityMs()" in body
    assert "resetOtaAfterFailedUpload()" in body
    assert "closeWifiOtaWindow(" in body
    assert "appendWebLog(" in body
    # 超时常量定义在 WifiConsoleTypes.h
    assert "WIFI_OTA_IDLE_TIMEOUT_MS = 60000" in WIFI_CONSOLE_TYPES_H


# --- WIFI|ssid|password 明文脱敏 ---

def test_redact_wireless_console_line_covers_wifi_provisioning():
    body = _function_body(WIRELESS_CONSOLE_CPP, r"String redactWirelessConsoleLine\(const String& line\)")
    assert 'startsWith("WIFI|")' in body
    assert "<redacted>" in body


# --- /api/wifi-sta apply_pending 契约字段 ---

def test_wifi_sta_json_exposes_apply_pending():
    body = _function_body(WEB_SERVER_CPP, r"static String wifiStaJson\(\)")
    assert '\\"apply_pending\\"' in body
    assert "staApplyPending" in body


def test_sta_set_clears_stale_error_before_deferred_apply():
    body = _function_body(WEB_SERVER_CPP, r"static void handleWifiWebStaSet\(\)")
    assert "clearWifiStaLastError()" in body
    assert body.index("clearWifiStaLastError()") < body.index("scheduleWifiStaApply()")
