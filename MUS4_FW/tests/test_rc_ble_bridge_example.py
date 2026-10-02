"""RC_BLE_Bridge 示例固件（examples/RC_BLE_Bridge）静态守护。

该示例把 CT-8B（F-08A 接收机）的 PWM 桥接成 BLE HID 手柄「Gamepad MU02」，
供 Mac 蓝牙配对后在 DD 页面当手柄输入源（含模拟器）。这些断言守护：
校准值与车端 FirmwareConfig.h 一致、失控保护、50Hz 节流、油门反向等关键约定。
"""

import pathlib


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
BRIDGE_SKETCH = PROJECT_ROOT / "examples" / "RC_BLE_Bridge" / "RC_BLE_Bridge.ino"
BRIDGE_README = PROJECT_ROOT / "examples" / "RC_BLE_Bridge" / "README.md"
FIRMWARE_CONFIG = PROJECT_ROOT / "libraries" / "mus4_core" / "src" / "FirmwareConfig.h"


def bridge_source() -> str:
    return BRIDGE_SKETCH.read_text(encoding="utf-8")


def test_rc_ble_bridge_files_exist():
    assert BRIDGE_SKETCH.exists()
    assert BRIDGE_README.exists()


def test_rc_ble_bridge_device_identity():
    source = bridge_source()

    # DD 侧按 padId 记忆校准，设备名必须固定
    assert 'BleGamepad bleGamepad("Gamepad MU02", "Espressif", 100)' in source
    assert "#include <BleGamepad.h>" in source


def test_rc_ble_bridge_pins_and_calibration_match_car_firmware():
    source = bridge_source()
    config = FIRMWARE_CONFIG.read_text(encoding="utf-8")

    # 引脚与车端一致（接收机在两套硬件间搬动时不用改线序认知）
    assert "#define CH1_PIN 36" in source
    assert "#define CH2_PIN 39" in source

    # 校准值必须与车端 FirmwareConfig.h 完全一致（同一台 CT-8B 的实测值）
    for line in [
        "RC_STEERING_MIN 872", "RC_STEERING_MID 1488", "RC_STEERING_MAX 2113",
        "RC_THROTTLE_MIN 888", "RC_THROTTLE_MID 1493", "RC_THROTTLE_MAX 2149",
        "RC_PWM_MIN 800", "RC_PWM_MAX 2200",
    ]:
        assert f"#define {line}" in source
        name, value = line.rsplit(" ", 1)
        assert f"#define {name} {value}" in config


def test_rc_ble_bridge_failsafe_and_rate():
    source = bridge_source()

    # 失控保护：超时判失控 + 存活判定 + 回中位
    assert "RC_SIGNAL_TIMEOUT_US 1000000UL" in source
    assert "rcChannelAlive" in source

    # 轴量程中位居中 + 50Hz 发送节流
    assert "AXIS_MID 16384" in source
    assert "SEND_INTERVAL_MS 20" in source

    # 油门按手柄惯例反向（前推=轴最小值），与 DD 默认 z-axis 预设 invert=true 配套
    assert "AXIS_MAX - mapRcAxis" in source

    # 分段线性映射（中位精确居中）
    assert "mapRcAxis" in source

    # 手柄轴落点与车端 GamepadMode 约定一致：转向右摇杆 X、油门左摇杆 Y
    assert "bleGamepad.setLeftThumb(0, ly)" in source
    assert "bleGamepad.setRightThumb(lx, 0)" in source


def test_rc_ble_bridge_ota_roundtrip_channels():
    """桥刷到车上 ESP32 使用后，必须保证能无线刷回车固件（OTA 往返）：
    ArduinoOTA（3232，密码 mus4-debug）+ HTTP /update（80）双通道 +
    STA 失败自动开兜底 AP（MUS4-RC-Bridge）+ 空 POST 不重启（对齐车端门禁语义）。"""

    source = bridge_source()

    # 回刷通道 1：ArduinoOTA
    assert '#define OTA_HOSTNAME "mus4-rc-bridge"' in source
    assert '#define OTA_PASSWORD "mus4-debug"' in source
    assert "ArduinoOTA.begin()" in source
    assert "ArduinoOTA.handle()" in source

    # 回刷通道 2：HTTP /update（空 POST 不重启，与车端 v1.7.x 门禁语义一致）
    assert 'otaServer.on("/update", HTTP_POST' in source
    assert "Update.begin(UPDATE_SIZE_UNKNOWN)" in source
    assert "NACK:NO_UPLOAD" in source
    assert "ACK:UPDATE_OK" in source
    assert "ESP.restart()" in source

    # 兜底 AP：STA 失败也能刷回
    assert '#define BRIDGE_AP_SSID "MUS4-RC-Bridge"' in source
    assert "WiFi.softAP(BRIDGE_AP_SSID)" in source
    assert "WIFI_STA_TIMEOUT_MS 15000" in source

    # WiFi 凭据复用本机密钥文件（gitignore 不入库），缺文件可编译
    assert '__has_include("WirelessSecrets.h")' in source
    assert '#define WIFI_STA_SSID ""' in source
