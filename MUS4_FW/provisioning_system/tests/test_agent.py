import unittest
from unittest.mock import patch, MagicMock, call
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../linux_agent')))

from wifi_manager import WifiManager
from serial_comm import SerialComm
from agent import ProvisioningAgent


class TestWifiManager(unittest.TestCase):
    # connect() 现采用两步式 profile 创建：
    #   1. nmcli connection delete <ssid>          （清理残留 profile）
    #   2. nmcli connection add type wifi ...       （显式指定 wifi-sec.key-mgmt wpa-psk）
    #   3. nmcli --wait 30 connection up <ssid>     （激活）
    #   4. ip -4 addr show <iface>                  （轮询 DHCP）
    # 原因：nmcli 1.54 的 `device wifi connect <ssid> password <pwd>` 不会自动推断
    # key-mgmt，报 "802-11-wireless-security.key-mgmt: 缺少属性"；改用 connection add
    # 显式指定 key-mgmt 可绕过该缺陷。

    def _ok(self, stdout=""):
        m = MagicMock()
        m.returncode = 0
        m.stdout = stdout
        m.stderr = ""
        return m

    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_success(self, mock_run):
        # delete / add / up 成功 / ip addr 首次即有 inet
        mock_run.side_effect = [
            self._ok(),
            self._ok(),
            self._ok(),
            self._ok("inet 192.168.1.100/24 brd 192.168.1.255 scope global dynamic wlan0"),
        ]

        wm = WifiManager('wlan0')
        success, result = wm.connect('newhome_iot', 'wxl922922')

        self.assertTrue(success)
        self.assertEqual(result, '192.168.1.100')

    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_up_failure(self, mock_run):
        # delete / add 成功，但 connection up 失败（如密码错误、信号差）
        mock_up = MagicMock()
        mock_up.returncode = 1
        mock_up.stderr = "Error: Connection activation failed."

        mock_run.side_effect = [self._ok(), self._ok(), mock_up]

        wm = WifiManager('wlan0')
        success, result = wm.connect('wrong_ssid', 'wrong_pass')

        self.assertFalse(success)
        self.assertEqual(result, '连接失败或超时')

    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_add_failure(self, mock_run):
        # connection add 失败（如 nmcli 拒绝创建 profile）应单独回报
        mock_add = MagicMock()
        mock_add.returncode = 1
        mock_add.stderr = "Error: invalid ssid"

        mock_run.side_effect = [self._ok(), mock_add]

        wm = WifiManager('wlan0')
        success, result = wm.connect('bad', 'pass')

        self.assertFalse(success)
        self.assertEqual(result, '创建连接配置失败')

    @patch('wifi_manager.subprocess.run')
    @patch('wifi_manager.time.sleep')
    def test_ip_polling_waits_for_dhcp(self, _mock_sleep, mock_run):
        # up 成功，但首次 ip addr 无 inet，第二次才有
        mock_run.side_effect = [
            self._ok(), self._ok(), self._ok(),
            self._ok(""),  # 尚未分配 IP
            self._ok("inet 10.0.0.5/24 brd 10.0.0.255 scope global dynamic wlan0"),
        ]

        wm = WifiManager('wlan0')
        success, result = wm.connect('ssid', 'pass')

        self.assertTrue(success)
        self.assertEqual(result, '10.0.0.5')
        # ip addr 至少被查询 2 次（轮询生效）
        self.assertGreaterEqual(mock_run.call_count, 5)

    @patch('wifi_manager.time.time')
    @patch('wifi_manager.subprocess.run')
    @patch('wifi_manager.time.sleep')
    def test_ip_polling_timeout(self, _mock_sleep, mock_run, mock_time):
        # up 成功，但 ip addr 始终无 inet → 轮询超时返回失败
        # time.time(): deadline=t0+1；循环1 t=0<1 查ip；循环2 t=0<1 查ip；循环3 t=2>1 退出
        mock_time.side_effect = [0, 0, 0, 2]
        mock_run.side_effect = [
            self._ok(), self._ok(), self._ok(),
            self._ok(""), self._ok(""),
        ]

        wm = WifiManager('wlan0')
        wm.ip_wait_timeout = 1
        wm.ip_wait_interval = 0.1
        success, result = wm.connect('ssid', 'pass')

        self.assertFalse(success)
        self.assertEqual(result, '无法获取IP地址')

    @patch('wifi_manager.subprocess.run')
    def test_connect_add_specifies_wpa_psk_key_mgmt(self, mock_run):
        # 核心断言：connection add 必须显式指定 wifi-sec.key-mgmt wpa-psk，
        # 否则 nmcli 1.54 不推断 key-mgmt 导致 "缺少属性" 错误
        mock_run.side_effect = [
            self._ok(), self._ok(), self._ok(),
            self._ok("inet 192.168.1.1/24 brd 192.168.1.255 scope global wlan0"),
        ]

        wm = WifiManager('wlan0')
        wm.connect("TestSSID", "testpass123")

        add_calls = [c for c in mock_run.call_args_list
                     if c.args and isinstance(c.args[0], list)
                     and 'nmcli' in c.args[0] and 'add' in c.args[0]]
        self.assertTrue(add_calls, "应存在 nmcli connection add 调用")
        cmd = add_calls[0].args[0]
        self.assertIn("wifi-sec.key-mgmt", cmd, "必须显式指定 wifi-sec.key-mgmt")
        idx = cmd.index("wifi-sec.key-mgmt")
        self.assertEqual(cmd[idx + 1], "wpa-psk", "key-mgmt 应为 wpa-psk")
        # 密码通过 wifi-sec.psk 传入
        self.assertIn("wifi-sec.psk", cmd)
        psk_idx = cmd.index("wifi-sec.psk")
        self.assertEqual(cmd[psk_idx + 1], "testpass123")

    @patch('wifi_manager.subprocess.run')
    def test_connect_uses_arg_list_not_shell(self, mock_run):
        # 验证所有 nmcli 调用均使用参数列表而非 shell=True，避免注入
        mock_run.side_effect = [
            self._ok(), self._ok(), self._ok(),
            self._ok("inet 192.168.1.1/24 brd 192.168.1.255 scope global wlan0"),
        ]

        wm = WifiManager('wlan0')
        wm.connect("my'ssid", "pa' ss")

        for c in mock_run.call_args_list:
            if c.args and isinstance(c.args[0], list) and 'nmcli' in c.args[0]:
                self.assertIsInstance(c.args[0], list)
                self.assertNotIn('shell', c.kwargs)  # 不显式 shell=True

    @patch('wifi_manager.subprocess.run')
    def test_nmcli_wait_is_global_option_on_connection_up(self, mock_run):
        # --wait 是 nmcli 全局选项，必须放在子命令 connection 之前；
        # 放在末尾会被 nmcli 当作额外参数拒绝。
        mock_run.side_effect = [
            self._ok(), self._ok(), self._ok(),
            self._ok("inet 192.168.1.1/24 brd 192.168.1.255 scope global wlan0"),
        ]

        wm = WifiManager('wlan0')
        wm.connect("ssid", "pass")

        up_calls = [c for c in mock_run.call_args_list
                    if c.args and isinstance(c.args[0], list)
                    and 'nmcli' in c.args[0] and 'connection' in c.args[0] and 'up' in c.args[0]]
        self.assertTrue(up_calls, "应存在 nmcli connection up 调用")
        cmd = up_calls[0].args[0]
        # 形如 ["nmcli", "--wait", "30", "connection", "up", <ssid>]
        self.assertEqual(cmd[1], "--wait", "--wait 必须紧跟 nmcli 作为全局选项")
        self.assertEqual(cmd[2], "30")
        self.assertEqual(cmd[3], "connection", "connection 子命令应在 --wait 之后")


class TestProvisioningAgentE2E(unittest.TestCase):
    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_end_to_end_success_flow(self, MockSerialComm, MockWifiManager):
        mock_serial = MockSerialComm.return_value
        mock_wifi = MockWifiManager.return_value

        mock_serial.read_line.side_effect = ["WIFI|newhome_iot|wxl922922", ""]
        mock_wifi.connect.return_value = (True, "192.168.1.150")

        agent = ProvisioningAgent()
        agent.handle_provisioning_request("WIFI|newhome_iot|wxl922922")

        mock_wifi.disconnect_ap.assert_called_once()
        mock_wifi.connect.assert_called_with("newhome_iot", "wxl922922")
        mock_serial.write_line.assert_any_call("STATUS|connecting")
        mock_serial.write_line.assert_any_call("OK|192.168.1.150")

    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_end_to_end_fail_flow(self, MockSerialComm, MockWifiManager):
        mock_serial = MockSerialComm.return_value
        mock_wifi = MockWifiManager.return_value

        mock_wifi.connect.return_value = (False, "连接超时")

        agent = ProvisioningAgent()
        agent.handle_provisioning_request("WIFI|unknown|123")

        mock_serial.write_line.assert_any_call("FAIL|连接超时")

    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_agent_default_port_is_ttyS6(self, MockSerialComm, MockWifiManager):
        # 验证默认串口端口为 /dev/ttyS6（Serial2 实际对接的 Linux 设备）
        ProvisioningAgent()
        args, _ = MockSerialComm.call_args
        self.assertEqual(args[0], "/dev/ttyS6")

    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_agent_port_override_via_arg(self, MockSerialComm, MockWifiManager):
        # 验证端口可通过构造参数覆盖
        ProvisioningAgent(port="/dev/ttyS99")
        args, _ = MockSerialComm.call_args
        self.assertEqual(args[0], "/dev/ttyS99")

    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_password_with_pipe_not_truncated(self, MockSerialComm, MockWifiManager):
        # WIFI|ssid|p|a|s|s 中密码含 |，split("|", 2) 后密码应完整
        mock_serial = MockSerialComm.return_value
        mock_wifi = MockWifiManager.return_value
        mock_wifi.connect.return_value = (True, "192.168.1.1")

        agent = ProvisioningAgent()
        agent.handle_provisioning_request("WIFI|myssid|p|a|s|s")

        mock_wifi.connect.assert_called_with("myssid", "p|a|s|s")


if __name__ == '__main__':
    unittest.main(verbosity=2)
