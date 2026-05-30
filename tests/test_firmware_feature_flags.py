import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MUS4_SKETCH = PROJECT_ROOT / "mus4.ino"


def test_websocket_curve_data_feature_is_enabled():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert re.search(r"^#define\s+ENABLE_WIFI_WEBSOCKET_TELEMETRY\b", source, re.MULTILINE)


def test_web_console_keeps_original_ui_and_direct_curve_path():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "MUS4 Web Console" in source
    assert "MUS4 Compact Console" not in source
    assert "pendingPoints.push" not in source
    assert "const interp={...prev}" not in source
    assert "chartLatencyMs=160" not in source
    assert "ws.send('ping')" not in source
    assert "\"pong\"" not in source


def test_diagnostic_code_is_not_built_by_default():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert re.search(r"^//\s*#define\s+ENABLE_DIAGNOSTIC_COMMANDS\b", source, re.MULTILINE)
    assert re.search(r"^//\s*#define\s+ENABLE_BOOT_STEERING_SELF_TEST\b", source, re.MULTILINE)


def test_web_console_tub_recorder_is_browser_side_and_reuses_telemetry_points():
    source = MUS4_SKETCH.read_text(encoding="utf-8")

    assert "mus4.web_data_point.tub.v1" in source
    assert "tubRecording" in source
    assert "tubSamples" in source
    assert "startTubRecording" in source
    assert "stopTubRecording" in source
    assert "downloadTubJson" in source
    assert "captureTubPoint" in source
    assert "handleDataPayload" in source
    assert "captureTubPoint(latest)" in source
    assert "TUB_MAX_SAMPLES" in source
    assert "LittleFS" not in source
    assert "SPIFFS" not in source
