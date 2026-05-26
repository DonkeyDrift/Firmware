import importlib.util
import pathlib
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "wireless_console_policy.py"
SPEC = importlib.util.spec_from_file_location("wireless_console_policy", MODULE_PATH)
POLICY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(POLICY)


class TestWirelessConsolePolicy(unittest.TestCase):
    def test_allows_public_status_without_authentication(self):
        self.assertTrue(POLICY.is_wireless_command_allowed("STATUS", authenticated=False, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("PING", authenticated=False, park_locked=False))

    def test_blocks_control_command_without_authentication(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("10:20", authenticated=False, park_locked=True))

    def test_allows_control_command_after_authentication(self):
        self.assertTrue(POLICY.is_wireless_command_allowed("10:20", authenticated=True, park_locked=False))

    def test_requires_park_locked_for_diagnostic_commands(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("TEST", authenticated=True, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("TEST", authenticated=True, park_locked=True))

    def test_auth_accepts_configured_password_only(self):
        self.assertTrue(POLICY.authenticate_wireless_command("AUTH:mus4-debug", "mus4-debug"))
        self.assertFalse(POLICY.authenticate_wireless_command("AUTH:wrong", "mus4-debug"))

    def test_requires_authentication_and_park_locked_for_ota_open(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("ENABLE_OTA", authenticated=False, park_locked=True))
        self.assertFalse(POLICY.is_wireless_command_allowed("ENABLE_OTA", authenticated=True, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("ENABLE_OTA", authenticated=True, park_locked=True))

    def test_requires_authentication_for_ota_status_and_close(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("OTA_STATUS", authenticated=False, park_locked=True))
        self.assertTrue(POLICY.is_wireless_command_allowed("OTA_STATUS", authenticated=True, park_locked=False))
        self.assertFalse(POLICY.is_wireless_command_allowed("DISABLE_OTA", authenticated=False, park_locked=True))
        self.assertTrue(POLICY.is_wireless_command_allowed("DISABLE_OTA", authenticated=True, park_locked=False))

    def test_ota_window_active_before_deadline_only(self):
        self.assertFalse(POLICY.is_ota_window_active(now_ms=1000, deadline_ms=0))
        self.assertTrue(POLICY.is_ota_window_active(now_ms=1000, deadline_ms=2000))
        self.assertFalse(POLICY.is_ota_window_active(now_ms=2000, deadline_ms=2000))
        self.assertFalse(POLICY.is_ota_window_active(now_ms=2500, deadline_ms=2000))

    def test_line_buffer_emits_complete_lines_and_tracks_overflow(self):
        buffer = POLICY.WirelessLineBuffer(max_length=8)

        self.assertEqual(buffer.feed("ABC"), [])
        self.assertEqual(buffer.feed("\r\nDEF\n"), ["ABC", "DEF"])
        self.assertFalse(buffer.overflowed)

        self.assertEqual(buffer.feed("123456789\n"), ["12345678"])
        self.assertTrue(buffer.overflowed)


if __name__ == "__main__":
    unittest.main()
