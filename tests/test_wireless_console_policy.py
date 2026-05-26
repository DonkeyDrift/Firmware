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

    def test_describes_wifi_dual_mode_with_ap_always_available(self):
        self.assertEqual(
            POLICY.describe_wifi_mode(ap_enabled=True, sta_configured=False, sta_connected=False),
            "ap",
        )
        self.assertEqual(
            POLICY.describe_wifi_mode(ap_enabled=True, sta_configured=True, sta_connected=False),
            "ap_sta_pending",
        )
        self.assertEqual(
            POLICY.describe_wifi_mode(ap_enabled=True, sta_configured=True, sta_connected=True),
            "ap_sta_connected",
        )

    def test_formats_network_status_with_unconfigured_sta(self):
        status = POLICY.format_network_status(
            ap_ip="192.168.4.1",
            web_port=80,
            sta_configured=False,
            sta_connected=False,
            sta_ip="",
        )

        self.assertEqual(
            status,
            "web_port=80 ap_ip=192.168.4.1 sta_configured=0 sta_connected=0 sta_ip=0.0.0.0",
        )

    def test_formats_network_status_with_connected_sta(self):
        status = POLICY.format_network_status(
            ap_ip="192.168.4.1",
            web_port=80,
            sta_configured=True,
            sta_connected=True,
            sta_ip="192.168.31.88",
        )

        self.assertEqual(
            status,
            "web_port=80 ap_ip=192.168.4.1 sta_configured=1 sta_connected=1 sta_ip=192.168.31.88",
        )

    def test_web_command_permissions_match_wireless_console(self):
        scenarios = [
            ("PING", False, False),
            ("STATUS", False, False),
            ("AUTH:mus4-debug", False, False),
            ("10:20", True, False),
            ("ENABLE_OTA", True, True),
            ("OTA_STATUS", True, False),
            ("TEST", True, True),
        ]
        for line, authenticated, park_locked in scenarios:
            with self.subTest(line=line):
                self.assertEqual(
                    POLICY.is_web_command_allowed(line, authenticated, park_locked),
                    POLICY.is_wireless_command_allowed(line, authenticated, park_locked),
                )

    def test_web_command_blocks_unauthenticated_control_and_unlocked_ota(self):
        self.assertFalse(POLICY.is_web_command_allowed("10:20", authenticated=False, park_locked=True))
        self.assertFalse(POLICY.is_web_command_allowed("ENABLE_OTA", authenticated=True, park_locked=False))


if __name__ == "__main__":
    unittest.main()
