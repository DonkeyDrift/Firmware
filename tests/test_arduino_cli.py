import importlib.util
import pathlib
import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "arduino-cli.py"
SPEC = importlib.util.spec_from_file_location("arduino_cli_module", MODULE_PATH)
ARDUINO_CLI = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ARDUINO_CLI)


def make_port(
    device,
    description="",
    manufacturer="",
    product="",
    hwid="",
    vid=None,
    pid=None,
    serial_number="",
    location="",
):
    return {
        "device": device,
        "description": description,
        "manufacturer": manufacturer,
        "product": product,
        "hwid": hwid,
        "serial_number": serial_number,
        "location": location,
        "vid": vid,
        "pid": pid,
    }


def make_automation(port="auto", serial_detection_cfg=None):
    automation = ARDUINO_CLI.ArduinoAutomation.__new__(ARDUINO_CLI.ArduinoAutomation)
    automation.logger = MagicMock()
    automation.port = port
    automation.serial_detection_cfg = serial_detection_cfg or {"enabled": True}
    automation.serial_detection_enabled = automation.serial_detection_cfg.get("enabled", True)
    automation.args = SimpleNamespace(
        port=None,
        input_file=None,
        build_path=None,
        config="config.yaml",
    )
    automation.arduino_cli = "arduino-cli"
    automation.fqbn = "esp32:esp32:esp32"
    automation.sketch = "mus4/mus4.ino"
    automation.config = {"default": {}}
    automation.os_type = "Windows"
    return automation


class TestSerialPortSelection(unittest.TestCase):
    def test_prefers_explicit_port_when_available(self):
        automation = make_automation(port="COM9")
        selected, reason = automation.select_best_port([
            make_port("COM10", description="USB-SERIAL CH340"),
            make_port("COM9", description="USB-SERIAL CH340"),
        ])

        self.assertEqual(selected["device"], "COM9")
        self.assertIn("命中指定端口", reason)

    def test_falls_back_to_auto_match_when_fixed_port_missing(self):
        automation = make_automation(port="COM9")
        selected, reason = automation.select_best_port([
            make_port(
                "COM11",
                description="USB-SERIAL CH340",
                manufacturer="wch",
                hwid="USB VID:PID=1A86:7523",
                vid=0x1A86,
                pid=0x7523,
            ),
            make_port("COM1", description="Standard Serial over Bluetooth"),
        ])

        self.assertEqual(selected["device"], "COM11")
        self.assertIn("自动匹配命中", reason)

    def test_rejects_ambiguous_candidates(self):
        automation = make_automation(port="auto")
        selected, reason = automation.select_best_port([
            make_port("COM11", description="USB-SERIAL CH340", hwid="USB VID:PID=1A86:7523"),
            make_port("COM12", description="USB-SERIAL CH340", hwid="USB VID:PID=1A86:7523"),
        ])

        self.assertIsNone(selected)
        self.assertIn("多个同分候选串口", reason)

    def test_uses_single_port_as_safe_fallback(self):
        automation = make_automation(port="auto")
        selected, reason = automation.select_best_port([
            make_port("COM7", description="Unknown Device"),
        ])

        self.assertEqual(selected["device"], "COM7")
        self.assertIn("仅检测到一个串口", reason)

    def test_preferred_description_keyword_breaks_tie(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        selected, reason = automation.select_best_port([
            make_port("COM19", description="USB-Enhanced-SERIAL-B CH342 (COM19)", manufacturer="wch.cn"),
            make_port("COM20", description="USB-Enhanced-SERIAL-A CH342 (COM20)", manufacturer="wch.cn"),
        ])

        self.assertEqual(selected["device"], "COM20")
        self.assertIn("自动匹配命中", reason)

    def test_disables_auto_match_when_configured(self):
        automation = make_automation(
            port="COM9",
            serial_detection_cfg={"enabled": False},
        )
        selected, reason = automation.select_best_port([
            make_port("COM11", description="USB-SERIAL CH340"),
        ])

        self.assertIsNone(selected)
        self.assertIn("已禁用自动匹配", reason)

    def test_build_upload_port_attempts_adds_sibling_port_fallback(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        automation.enumerate_serial_ports = MagicMock(return_value=[
            make_port("COM19", description="USB-Enhanced-SERIAL-B CH342 (COM19)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM20", description="USB-Enhanced-SERIAL-A CH342 (COM20)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM6", description="Bluetooth", manufacturer="Microsoft"),
        ])

        attempts, error = automation.build_upload_port_attempts()

        self.assertIsNone(error)
        self.assertEqual([item["port"]["device"] for item in attempts], ["COM20", "COM19"])

    def test_upload_retries_with_sibling_port_after_failure(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        automation.enumerate_serial_ports = MagicMock(return_value=[
            make_port("COM19", description="USB-Enhanced-SERIAL-B CH342 (COM19)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM20", description="USB-Enhanced-SERIAL-A CH342 (COM20)", manufacturer="wch.cn", serial_number="ABC"),
        ])
        automation.run_command = MagicMock(side_effect=[(False, "busy"), (True, "ok")])

        result = automation.upload()

        self.assertTrue(result)
        self.assertEqual(automation.port, "COM19")
        self.assertEqual(automation.run_command.call_count, 2)
        first_cmd = automation.run_command.call_args_list[0].args[0]
        second_cmd = automation.run_command.call_args_list[1].args[0]
        self.assertIn("COM20", first_cmd)
        self.assertIn("COM19", second_cmd)


if __name__ == "__main__":
    unittest.main(verbosity=2)
