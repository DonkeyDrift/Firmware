import importlib.util
import pathlib
import tempfile
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
    automation.config_path = str(PROJECT_ROOT / "config.yaml")
    automation.libraries_path = "libraries"
    automation.os_type = "Windows"
    automation.serial_state_file = str(PROJECT_ROOT / ".tmp_serial_state_test.json")
    return automation


class TestSketchDiscovery(unittest.TestCase):
    def make_resolver(self, root):
        automation = ARDUINO_CLI.ArduinoAutomation.__new__(ARDUINO_CLI.ArduinoAutomation)
        automation.logger = MagicMock()
        automation.config_path = str(root / "config.yaml")
        return automation

    def test_falls_back_to_single_root_sketch_when_configured_sketch_is_missing(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            (root / "MUS4_FW.ino").write_text("", encoding="utf-8")
            automation = self.make_resolver(root)

            selected = automation.resolve_sketch_path("mus4.ino")

        self.assertEqual(pathlib.Path(selected).name, "MUS4_FW.ino")
        automation.logger.warning.assert_called()

    def test_rejects_multiple_root_sketches_when_configured_sketch_is_missing(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            (root / "MUS4_FW.ino").write_text("", encoding="utf-8")
            (root / "other.ino").write_text("", encoding="utf-8")
            automation = self.make_resolver(root)

            with self.assertRaises(SystemExit) as raised:
                automation.resolve_sketch_path("mus4.ino")

        self.assertEqual(raised.exception.code, 3)
        automation.logger.error.assert_called()


class TestCompileCommand(unittest.TestCase):
    def test_compile_uses_local_libraries_when_directory_exists(self):
        automation = make_automation()
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            sketch = root / "mus4.ino"
            libraries = root / "libraries"
            build = root / "build"
            sketch.write_text("", encoding="utf-8")
            libraries.mkdir()
            automation.sketch = str(sketch)
            automation.config_path = str(root / "config.yaml")
            automation.config = {"default": {"libraries_path": "libraries", "build_path": str(build)}}
            automation.args.config = str(root / "config.yaml")
            automation.run_command = MagicMock(return_value=(True, "ok"))

            result = automation.compile()

        self.assertTrue(result)
        command = automation.run_command.call_args.args[0]
        self.assertIn("--libraries", command)
        self.assertIn(str(libraries), command)

    def test_compile_skips_local_libraries_when_directory_is_missing(self):
        automation = make_automation()
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            sketch = root / "mus4.ino"
            build = root / "build"
            sketch.write_text("", encoding="utf-8")
            automation.sketch = str(sketch)
            automation.config_path = str(root / "config.yaml")
            automation.config = {"default": {"libraries_path": "libraries", "build_path": str(build)}}
            automation.args.config = str(root / "config.yaml")
            automation.run_command = MagicMock(return_value=(True, "ok"))

            result = automation.compile()

        self.assertTrue(result)
        command = automation.run_command.call_args.args[0]
        self.assertNotIn("--libraries", command)


class TestPrecompiledFirmwareSelection(unittest.TestCase):
    def test_replaces_bootloader_fragment_with_main_firmware_when_available(self):
        automation = make_automation()
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            main_firmware = root / "mus4.ino.bin"
            bootloader = root / "mus4.ino.bootloader.bin"
            main_firmware.write_bytes(b"app")
            bootloader.write_bytes(b"bootloader")

            selected = automation.normalize_precompiled_input_file(str(bootloader))

        self.assertEqual(selected, str(main_firmware))

    def test_replaces_flashed_fragment_with_main_firmware_when_available(self):
        automation = make_automation()
        with tempfile.TemporaryDirectory() as tmpdir:
            root = pathlib.Path(tmpdir)
            main_firmware = root / "MUS4_FW.ino.bin"
            flashed_partition = root / "MUS4_FW.ino.partitions_flashed.bin"
            main_firmware.write_bytes(b"app")
            flashed_partition.write_bytes(b"partition")

            selected = automation.normalize_precompiled_input_file(str(flashed_partition))

        self.assertEqual(selected, str(main_firmware))

    def test_keeps_main_firmware_input_file(self):
        automation = make_automation()
        selected = automation.normalize_precompiled_input_file("build_wsl/mus4.ino.bin")

        self.assertEqual(selected, "build_wsl/mus4.ino.bin")


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
        automation.get_last_success_port = MagicMock(return_value="")
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
        automation.get_last_success_port = MagicMock(return_value="")
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

    def test_build_upload_port_attempts_prefers_last_success_port(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        automation.get_last_success_port = MagicMock(return_value="COM19")
        automation.enumerate_serial_ports = MagicMock(return_value=[
            make_port("COM19", description="USB-Enhanced-SERIAL-B CH342 (COM19)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM20", description="USB-Enhanced-SERIAL-A CH342 (COM20)", manufacturer="wch.cn", serial_number="ABC"),
        ])

        attempts, error = automation.build_upload_port_attempts()

        self.assertIsNone(error)
        self.assertEqual([item["port"]["device"] for item in attempts], ["COM19", "COM20"])

    def test_upload_records_success_port_after_fallback(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        automation.get_last_success_port = MagicMock(return_value="COM20")
        automation.save_last_success_port = MagicMock()
        automation.enumerate_serial_ports = MagicMock(return_value=[
            make_port("COM19", description="USB-Enhanced-SERIAL-B CH342 (COM19)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM20", description="USB-Enhanced-SERIAL-A CH342 (COM20)", manufacturer="wch.cn", serial_number="ABC"),
        ])
        automation.run_command = MagicMock(side_effect=[(False, "busy"), (True, "ok")])

        result = automation.upload()

        self.assertTrue(result)
        self.assertEqual(automation.port, "COM19")
        automation.save_last_success_port.assert_called_once_with("COM19")

    def test_resolve_port_prefers_last_success_port_for_serial_mode(self):
        automation = make_automation(
            port="auto",
            serial_detection_cfg={
                "enabled": True,
                "preferred_description_keywords": ["serial-a"],
            },
        )
        automation.get_last_success_port = MagicMock(return_value="COM27")
        automation.enumerate_serial_ports = MagicMock(return_value=[
            make_port("COM26", description="USB-Enhanced-SERIAL-A CH342 (COM26)", manufacturer="wch.cn", serial_number="ABC"),
            make_port("COM27", description="USB-Enhanced-SERIAL-B CH342 (COM27)", manufacturer="wch.cn", serial_number="ABC"),
        ])

        resolved = automation.resolve_port(required=True)

        self.assertEqual(resolved, "COM27")
        self.assertEqual(automation.port, "COM27")


class TestSerialPortState(unittest.TestCase):
    def test_save_and_load_last_success_port(self):
        automation = make_automation(port="auto")
        with tempfile.TemporaryDirectory() as tmp:
            automation.serial_state_file = str(pathlib.Path(tmp) / "serial_state.json")
            automation.save_last_success_port("COM27")
            loaded = automation.get_last_success_port()
        self.assertEqual(loaded, "COM27")


class TestOtaUploadTooling(unittest.TestCase):
    def test_prefers_explicit_espota_tool(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            explicit = root / "custom_espota.py"
            env_tool = root / "env_espota.py"
            explicit.write_text("", encoding="utf-8")
            env_tool.write_text("", encoding="utf-8")

            selected = ARDUINO_CLI.find_espota_tool(
                explicit_path=str(explicit),
                env={"ESPOTA_PY": str(env_tool)},
            )

        self.assertEqual(selected, str(explicit))

    def test_uses_espota_tool_from_environment(self):
        with tempfile.TemporaryDirectory() as tmp:
            env_tool = pathlib.Path(tmp) / "espota.py"
            env_tool.write_text("", encoding="utf-8")

            selected = ARDUINO_CLI.find_espota_tool(env={"ESPOTA_PY": str(env_tool)})

        self.assertEqual(selected, str(env_tool))

    def test_discovers_newest_arduino15_espota_tool(self):
        with tempfile.TemporaryDirectory() as tmp:
            local_appdata = pathlib.Path(tmp)
            old_tool = local_appdata / "Arduino15" / "packages" / "esp32" / "hardware" / "esp32" / "3.2.0" / "tools" / "espota.py"
            new_tool = local_appdata / "Arduino15" / "packages" / "esp32" / "hardware" / "esp32" / "3.3.8-cn" / "tools" / "espota.py"
            old_tool.parent.mkdir(parents=True)
            new_tool.parent.mkdir(parents=True)
            old_tool.write_text("", encoding="utf-8")
            new_tool.write_text("", encoding="utf-8")

            selected = ARDUINO_CLI.find_espota_tool(
                env={},
                local_appdata=str(local_appdata),
                home=str(local_appdata / "home"),
            )

        self.assertEqual(selected, str(new_tool))

    def test_builds_espota_command(self):
        command = ARDUINO_CLI.build_espota_command(
            python_exe="python",
            espota_tool="C:/tools/espota.py",
            host="192.168.4.1",
            port=3232,
            password="mus4-debug",
            bin_path="C:/build/mus4.ino.bin",
        )

        self.assertEqual(command, [
            "python",
            "C:/tools/espota.py",
            "-i", "192.168.4.1",
            "-p", "3232",
            "-a", "mus4-debug",
            "-f", "C:/build/mus4.ino.bin",
            "--progress",
        ])

    def test_parses_espota_upload_progress(self):
        progress = ARDUINO_CLI.parse_espota_progress_line("Uploading: [==========          ] 50%")

        self.assertEqual(progress, "[==========          ] 50.0%")

    def test_splits_progress_chunks_on_carriage_return(self):
        chunks = list(ARDUINO_CLI.split_progress_chunks("Uploading: [=                   ] 5%\rUploading: [==========          ] 50%\r"))

        self.assertEqual(chunks, [
            "Uploading: [=                   ] 5%",
            "Uploading: [==========          ] 50%",
        ])

    def test_ota_upload_uses_espota_progress_parser(self):
        automation = make_automation(port="auto")
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            firmware = root / "mus4.ino.bin"
            espota_tool = root / "espota.py"
            firmware.write_bytes(b"app")
            espota_tool.write_text("", encoding="utf-8")
            automation.args = SimpleNamespace(
                port=None,
                input_file=str(firmware),
                build_path=None,
                config="config.yaml",
                ota_host="192.168.4.1",
                ota_port=3232,
                ota_password="mus4-debug",
                espota_tool=str(espota_tool),
            )
            automation.run_command = MagicMock(return_value=(True, "ok"))

            result = automation.ota_upload()

        self.assertTrue(result)
        self.assertIs(automation.run_command.call_args.kwargs["progress_parser"], ARDUINO_CLI.parse_espota_progress_line)

    def test_ota_upload_does_not_resolve_serial_ports(self):
        automation = make_automation(port="auto")
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            firmware = root / "mus4.ino.bin"
            espota_tool = root / "espota.py"
            firmware.write_bytes(b"app")
            espota_tool.write_text("", encoding="utf-8")
            automation.args = SimpleNamespace(
                port=None,
                input_file=str(firmware),
                build_path=None,
                config="config.yaml",
                ota_host="192.168.4.1",
                ota_port=3232,
                ota_password="mus4-debug",
                espota_tool=str(espota_tool),
            )
            automation.build_upload_port_attempts = MagicMock()
            automation.run_command = MagicMock(return_value=(True, "ok"))

            result = automation.ota_upload()

        self.assertTrue(result)
        automation.build_upload_port_attempts.assert_not_called()
        command = automation.run_command.call_args.args[0]
        self.assertIn(str(espota_tool), command)
        self.assertIn(str(firmware), command)

    def test_run_treats_ota_as_upload_action(self):
        automation = make_automation(port="auto")
        automation.args = SimpleNamespace(
            list_ports=False,
            compile=False,
            upload=False,
            serial=False,
            regress_reset=False,
            regress_count=10,
            ota=True,
        )
        automation.ota_upload = MagicMock(return_value=True)
        automation.upload = MagicMock()
        automation.auto_reset = MagicMock()
        automation.monitor = MagicMock()

        automation.run()

        automation.ota_upload.assert_called_once_with()
        automation.upload.assert_not_called()
        automation.auto_reset.assert_not_called()
        automation.monitor.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
