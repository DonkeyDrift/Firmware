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

    def test_local_ota_open_requires_password_and_park_locked(self):
        self.assertFalse(POLICY.is_local_ota_open_command_allowed("ENABLE_OTA", "mus4-debug", park_locked=True))
        self.assertFalse(POLICY.is_local_ota_open_command_allowed("ENABLE_OTA:wrong", "mus4-debug", park_locked=True))
        self.assertTrue(POLICY.is_local_ota_open_command_allowed("ENABLE_OTA:mus4-debug", "mus4-debug", park_locked=False))
        self.assertTrue(POLICY.is_local_ota_open_command_allowed("ENABLE_OTA:mus4-debug", "mus4-debug", park_locked=True))

    def test_local_ota_status_and_close_are_maintenance_commands(self):
        self.assertTrue(POLICY.is_local_ota_status_command("OTA_STATUS"))
        self.assertTrue(POLICY.is_local_ota_close_command("DISABLE_OTA"))
        self.assertFalse(POLICY.is_local_ota_status_command("STATUS"))
        self.assertFalse(POLICY.is_local_ota_close_command("ENABLE_OTA:mus4-debug"))

    def test_ota_window_active_before_deadline_only(self):
        self.assertFalse(POLICY.is_ota_window_active(now_ms=1000, deadline_ms=0))
        self.assertTrue(POLICY.is_ota_window_active(now_ms=1000, deadline_ms=2000))
        self.assertFalse(POLICY.is_ota_window_active(now_ms=2000, deadline_ms=2000))
        self.assertFalse(POLICY.is_ota_window_active(now_ms=2500, deadline_ms=2000))

    def test_dev_mode_keeps_ota_window_active(self):
        self.assertTrue(POLICY.is_ota_window_active(now_ms=2500, deadline_ms=2000, dev_mode=True))
        self.assertTrue(POLICY.is_ota_window_active(now_ms=1000, deadline_ms=0, dev_mode=True))

    def test_serial1_telemetry_pauses_only_during_active_transfer(self):
        # v1.7.8 起：仅在 OTA 真正传输（in_progress=True）期间暂停 Serial1 遥测，
        # 避免 DEV ON 时 windowOpen 持续为 True 阻塞与上位机通信。Park Guard
        # 仍由 forceWifiOtaParkLocked() 在传输期内托底。
        self.assertTrue(POLICY.should_emit_serial1_telemetry(ota_window_open=False, ota_in_progress=False))
        self.assertTrue(POLICY.should_emit_serial1_telemetry(ota_window_open=True, ota_in_progress=False))
        self.assertFalse(POLICY.should_emit_serial1_telemetry(ota_window_open=False, ota_in_progress=True))
        self.assertFalse(POLICY.should_emit_serial1_telemetry(ota_window_open=True, ota_in_progress=True))

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
            ap_ssid="MUS4-ESP",
            sta_ssid="",
            free_heap=145000,
            min_free_heap=60000,
            ws_queue_full_skip=3,
            ws_max_backlog=12,
            ws_connects=2,
            ws_disconnects=1,
            web_update_dt_max=87,
            web_sample_dt_max=4,
            web_http_dt_max=51,
            web_ws_dt_max=32,
            http_status_count=9,
            http_log_count=25,
            http_data_count=0,
            http_cmd_count=1,
            http_status_dt_max=12,
            http_log_dt_max=2123,
            http_data_dt_max=4,
            http_cmd_dt_max=7,
        )

        self.assertEqual(
            status,
            "web_port=80 free_heap=145000 min_free_heap=60000 ws_queue_full_skip=3 ws_max_backlog=12 ws_connects=2 ws_disconnects=1 web_update_dt_max=87 web_sample_dt_max=4 web_http_dt_max=51 web_ws_dt_max=32 http_status_count=9 http_log_count=25 http_data_count=0 http_cmd_count=1 http_status_dt_max=12 http_log_dt_max=2123 http_data_dt_max=4 http_cmd_dt_max=7 ap_ssid=\"MUS4-ESP\" ap_ip=192.168.4.1 sta_configured=0 sta_connected=0 sta_ssid=\"\" sta_ip=0.0.0.0",
        )

    def test_formats_network_status_with_connected_sta(self):
        status = POLICY.format_network_status(
            ap_ip="192.168.4.1",
            web_port=80,
            sta_configured=True,
            sta_connected=True,
            sta_ip="192.168.31.88",
            ap_ssid="MUS4-ESP",
            sta_ssid="Home WiFi",
        )

        self.assertEqual(
            status,
            "web_port=80 free_heap=0 min_free_heap=0 ws_queue_full_skip=0 ws_max_backlog=0 ws_connects=0 ws_disconnects=0 web_update_dt_max=0 web_sample_dt_max=0 web_http_dt_max=0 web_ws_dt_max=0 http_status_count=0 http_log_count=0 http_data_count=0 http_cmd_count=0 http_status_dt_max=0 http_log_dt_max=0 http_data_dt_max=0 http_cmd_dt_max=0 ap_ssid=\"MUS4-ESP\" ap_ip=192.168.4.1 sta_configured=1 sta_connected=1 sta_ssid=\"Home WiFi\" sta_ip=192.168.31.88",
        )

    def test_formats_wifi_sta_state_with_failure_reason(self):
        state = POLICY.format_wifi_sta_state(
            configured=True,
            connected=False,
            timed_out=True,
            connecting=False,
            ssid="HomeWiFi",
            password_set=True,
            ap_ip="192.168.4.1",
            sta_ip="0.0.0.0",
            last_error="timeout",
            last_error_message="STA 连接超时，请检查 SSID、密码与路由器信号。",
        )

        self.assertEqual(
            state,
            {
                "configured": True,
                "connected": False,
                "timed_out": True,
                "connecting": False,
                "ssid": "HomeWiFi",
                "password_set": True,
                "ap_ip": "192.168.4.1",
                "sta_ip": "0.0.0.0",
                "last_error": "timeout",
                "last_error_message": "STA 连接超时，请检查 SSID、密码与路由器信号。",
            },
        )

    def test_formats_wifi_sta_state_clears_error_when_connected(self):
        state = POLICY.format_wifi_sta_state(
            configured=True,
            connected=True,
            timed_out=False,
            connecting=False,
            ssid="HomeWiFi",
            password_set=True,
            ap_ip="192.168.4.1",
            sta_ip="192.168.3.144",
            last_error="timeout",
            last_error_message="STA 连接超时，请检查 SSID、密码与路由器信号。",
        )

        self.assertEqual(state["last_error"], "")
        self.assertEqual(state["last_error_message"], "")
        self.assertEqual(state["sta_ip"], "192.168.3.144")

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

    def test_web_dev_mode_allows_ota_without_authentication_but_keeps_park_guard(self):
        self.assertFalse(POLICY.is_web_command_allowed("ENABLE_OTA", authenticated=False, park_locked=False, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("ENABLE_OTA", authenticated=False, park_locked=True, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("OTA_STATUS", authenticated=False, park_locked=False, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("DISABLE_OTA", authenticated=False, park_locked=False, dev_mode=True))

    def test_web_dev_mode_does_not_bypass_authentication_for_control_or_diagnostic(self):
        # DEV ON 仅放权 OTA / Web 配置 / 显示与日志切换 / WIFI_STA_*；
        # 控制命令、Park 锁定诊断命令仍要求认证。详见
        # docs/Plan/DEV模式影响面与运行逻辑映射.md §3 与
        # docs/Plan/DEV模式安全边界收敛RFC.md。
        self.assertFalse(POLICY.is_web_command_allowed("10:20", authenticated=False, park_locked=False, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("10:20", authenticated=False, park_locked=True, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("TEST", authenticated=False, park_locked=False, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("TEST", authenticated=False, park_locked=True, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("BENCH", authenticated=False, park_locked=True, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("REGRESS", authenticated=False, park_locked=True, dev_mode=True))
        self.assertFalse(POLICY.is_web_command_allowed("STEER_CAL", authenticated=False, park_locked=True, dev_mode=True))
        # 认证后仍按原有规则：控制命令允许、诊断命令需 Park 锁定。
        self.assertTrue(POLICY.is_web_command_allowed("10:20", authenticated=True, park_locked=False, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("TEST", authenticated=True, park_locked=True, dev_mode=True))

    def test_wifi_sta_status_is_public(self):
        self.assertTrue(POLICY.is_wireless_command_allowed("WIFI_STA_STATUS", authenticated=False, park_locked=False))

    def test_wifi_sta_config_requires_authentication(self):
        commands = [
            "WIFI_STA_SSID:HomeWiFi",
            "WIFI_STA_PASSWORD:secret123",
            "WIFI_STA_APPLY",
            "WIFI_STA_CLEAR",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertFalse(POLICY.is_wireless_command_allowed(command, authenticated=False, park_locked=True))
                self.assertTrue(POLICY.is_wireless_command_allowed(command, authenticated=True, park_locked=False))

    def test_web_dev_mode_allows_wifi_sta_config_without_authentication(self):
        self.assertTrue(POLICY.is_web_command_allowed("WIFI_STA_SSID:HomeWiFi", authenticated=False, park_locked=False, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("WIFI_STA_PASSWORD:secret123", authenticated=False, park_locked=False, dev_mode=True))
        self.assertTrue(POLICY.is_web_command_allowed("WIFI_STA_CLEAR", authenticated=False, park_locked=False, dev_mode=True))

    def test_redacts_sensitive_wireless_commands(self):
        self.assertEqual(POLICY.redact_wireless_console_line("AUTH:mus4-debug"), "AUTH:<redacted>")
        self.assertEqual(POLICY.redact_wireless_console_line("WIFI_STA_PASSWORD:secret123"), "WIFI_STA_PASSWORD:<redacted>")
        self.assertEqual(POLICY.redact_wireless_console_line("WIFI_STA_SSID:HomeWiFi"), "WIFI_STA_SSID:HomeWiFi")

    def test_tcp_console_ignores_dev_mode_ota_relaxation(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("ENABLE_OTA", authenticated=False, park_locked=False, dev_mode=True, origin="tcp"))
        self.assertFalse(POLICY.is_wireless_command_allowed("OTA_STATUS", authenticated=False, park_locked=False, dev_mode=True, origin="tcp"))

    def test_dev_mode_alone_does_not_force_park(self):
        self.assertFalse(POLICY.should_force_park_for_ota(park_guard_active=False, ota_in_progress=False, dev_mode=True))

    def test_ota_park_guard_forces_park_during_guard_or_transfer(self):
        self.assertFalse(POLICY.should_force_park_for_ota(park_guard_active=False, ota_in_progress=False))
        self.assertTrue(POLICY.should_force_park_for_ota(park_guard_active=True, ota_in_progress=False))
        self.assertTrue(POLICY.should_force_park_for_ota(park_guard_active=False, ota_in_progress=True))
        self.assertTrue(POLICY.should_force_park_for_ota(park_guard_active=True, ota_in_progress=False, dev_mode=True))

    def test_dev_mode_keeps_ota_window_without_park_guard(self):
        state = POLICY.dev_mode_ota_state(dev_mode=True, ota_window_open=False)

        self.assertTrue(state["ota_window_open"])
        self.assertFalse(state["park_guard_active"])

    def test_web_log_buffer_returns_incremental_entries_and_tracks_drops(self):
        buffer = POLICY.WebLogBuffer(capacity=2)

        first_seq = buffer.append(now_ms=100, source="web", line="> PING")
        second_seq = buffer.append(now_ms=120, source="cmd", line="PONG")

        self.assertEqual(first_seq, 1)
        self.assertEqual(second_seq, 2)
        self.assertEqual([entry["line"] for entry in buffer.since(0)], ["> PING", "PONG"])
        self.assertEqual([entry["line"] for entry in buffer.since(1)], ["PONG"])

        buffer.append(now_ms=140, source="web", line="> STATUS")

        self.assertEqual(buffer.dropped, 1)
        self.assertEqual([entry["seq"] for entry in buffer.since(0)], [2, 3])

    def test_formats_web_data_point_with_short_keys(self):
        point = POLICY.format_web_data_point(
            seq=7,
            now_ms=1234,
            control={"throttle": 10, "steering": -20, "mode": 1, "park": False},
            rc={"throttle": 1510, "steering": 1490, "ch1": 1490, "ch2": 1510, "ch3": 1000, "ch4": 1500, "ch5": 2000, "ch6": 1600},
            pilot={"throttle": 8, "steering": -18},
            sensor={"current_mA": 123.4, "voltage": 7.6, "gyroZ": -0.12},
            drift={"enabled": True, "active": True, "compensation": -12.5, "gyroZFiltered": 0.34},
        )

        self.assertEqual(
            point,
            {
                "seq": 7,
                "t": 1234,
                "thr": 10,
                "str": -20,
                "mode": 1,
                "park": 0,
                "rct": 1510,
                "rcs": 1490,
                "ch1": 1490,
                "ch2": 1510,
                "ch3": 1000,
                "ch4": 1500,
                "ch5": 2000,
                "ch6": 1600,
                "pt": 8,
                "ps": -18,
                "cur": 123.4,
                "vol": 7.6,
                "gz": -0.12,
                "de": 1,
                "da": 1,
                "dc": -12.5,
                "gzf": 0.34,
            },
        )

    def test_formats_web_data_point_defaults_drift_off(self):
        point = POLICY.format_web_data_point(
            seq=7,
            now_ms=1234,
            control={"throttle": 10, "steering": -20, "mode": 1, "park": False},
            rc={"throttle": 1510, "steering": 1490},
            pilot={"throttle": 8, "steering": -18},
            sensor={"current_mA": 123.4, "voltage": 7.6, "gyroZ": -0.12},
        )

        self.assertEqual(point["ch1"], 1490)
        self.assertEqual(point["ch2"], 1510)
        self.assertEqual(point["ch3"], 1500)
        self.assertEqual(point["ch4"], 1500)
        self.assertEqual(point["ch5"], 1000)
        self.assertEqual(point["ch6"], 1500)
        self.assertEqual(point["de"], 0)
        self.assertEqual(point["da"], 0)
        self.assertEqual(point["dc"], 0.0)
        self.assertEqual(point["gzf"], 0.0)

    def test_formats_tub_package_with_web_data_points(self):
        point = POLICY.format_web_data_point(
            seq=7,
            now_ms=1234,
            control={"throttle": 10, "steering": -20, "mode": 1, "park": False},
            rc={"throttle": 1510, "steering": 1490, "ch1": 1490, "ch2": 1510, "ch3": 1000, "ch4": 1500, "ch5": 2000, "ch6": 1600},
            pilot={"throttle": 8, "steering": -18},
            sensor={"current_mA": 123.4, "voltage": 7.6, "gyroZ": -0.12},
            drift={"enabled": True, "active": True, "compensation": -12.5, "gyroZFiltered": 0.34},
        )

        package = POLICY.format_tub_package(started_ms=1000, stopped_ms=3500, samples=[point])

        self.assertEqual(package["schema"], "mus4.web_data_point.tub.v1")
        self.assertEqual(package["source"], "mus4-web-console")
        self.assertEqual(package["started_ms"], 1000)
        self.assertEqual(package["stopped_ms"], 3500)
        self.assertEqual(package["count"], 1)
        self.assertEqual(package["samples"], [point])

    def test_tub_package_includes_all_rc_channels(self):
        point = POLICY.format_web_data_point(
            seq=8,
            now_ms=1500,
            control={"throttle": 0, "steering": 0, "mode": 0, "park": True},
            rc={"throttle": 1200, "steering": 1300, "ch1": 1300, "ch2": 1200, "ch3": 1100, "ch4": 1400, "ch5": 1700, "ch6": 1900},
            pilot={"throttle": 0, "steering": 0},
            sensor={"current_mA": 0.0, "voltage": 0.0, "gyroZ": 0.0},
        )

        package = POLICY.format_tub_package(started_ms=1500, stopped_ms=1500, samples=[point])
        sample = package["samples"][0]

        self.assertEqual([sample[f"ch{i}"] for i in range(1, 7)], [1300, 1200, 1100, 1400, 1700, 1900])

    def test_log_target_commands_require_authentication(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("LOG_WEB", authenticated=False, park_locked=True))
        self.assertFalse(POLICY.is_wireless_command_allowed("LOG_SERIAL", authenticated=False, park_locked=True))
        self.assertTrue(POLICY.is_wireless_command_allowed("LOG_WEB", authenticated=True, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("LOG_SERIAL", authenticated=True, park_locked=False))

    def test_select_log_target_defaults_to_web_when_wifi_enabled(self):
        self.assertEqual(POLICY.select_log_target(None, wifi_console_enabled=True), "web")

    def test_select_log_target_falls_back_to_serial_when_wifi_disabled(self):
        self.assertEqual(POLICY.select_log_target("web", wifi_console_enabled=False), "serial")
        self.assertEqual(POLICY.select_log_target(None, wifi_console_enabled=False), "serial")

    def test_select_log_target_serial_when_configured(self):
        self.assertEqual(POLICY.select_log_target("serial", wifi_console_enabled=True), "serial")


if __name__ == "__main__":
    unittest.main()
