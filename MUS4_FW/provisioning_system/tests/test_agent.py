import unittest
from unittest.mock import patch, MagicMock, call
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../linux_agent')))

from wifi_manager import WifiManager
from serial_comm import SerialComm
from agent import ProvisioningAgent


class TestWifiManager(unittest.TestCase):
    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_success(self, mock_run):
        # 模拟 nmcli delete / nmcli connect 成功 / ip addr 首次即有 inet
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0

        mock_ip = MagicMock()
        mock_ip.returncode = 0
        mock_ip.stdout = "inet 192.168.1.100/24 brd 192.168.1.255 scope global dynamic wlan0"

        mock_run.side_effect = [mock_delete, mock_nmcli, mock_ip]

        wm = WifiManager('wlan0')
        success, result = wm.connect('newhome_iot', 'wxl922922')

        self.assertTrue(success)
        self.assertEqual(result, '192.168.1.100')

    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_failure(self, mock_run):
        # 模拟 nmcli connect 失败 (如密码错误)
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 1
        mock_nmcli.stderr = "Error: Connection activation failed."

        mock_run.side_effect = [mock_delete, mock_nmcli]

        wm = WifiManager('wlan0')
        success, result = wm.connect('wrong_ssid', 'wrong_pass')

        self.assertFalse(success)
        self.assertEqual(result, '连接失败或超时')

    @patch('wifi_manager.subprocess.run')
    @patch('wifi_manager.time.sleep')
    def test_ip_polling_waits_for_dhcp(self, _mock_sleep, mock_run):
        # nmcli connect 成功，但首次 ip addr 无 inet，第二次才有
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0

        mock_ip_empty = MagicMock()
        mock_ip_empty.returncode = 0
        mock_ip_empty.stdout = ""  # 尚未分配 IP

        mock_ip_ok = MagicMock()
        mock_ip_ok.returncode = 0
        mock_ip_ok.stdout = "inet 10.0.0.5/24 brd 10.0.0.255 scope global dynamic wlan0"

        mock_run.side_effect = [mock_delete, mock_nmcli, mock_ip_empty, mock_ip_ok]

        wm = WifiManager('wlan0')
        success, result = wm.connect('ssid', 'pass')

        self.assertTrue(success)
        self.assertEqual(result, '10.0.0.5')
        # ip addr 至少被查询 2 次（轮询生效）
        self.assertGreaterEqual(mock_run.call_count, 4)

    @patch('wifi_manager.time.time')
    @patch('wifi_manager.subprocess.run')
    @patch('wifi_manager.time.sleep')
    def test_ip_polling_timeout(self, _mock_sleep, mock_run, mock_time):
        # nmcli 成功，但 ip addr 始终无 inet → 轮询超时返回失败
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0

        mock_ip_empty = MagicMock()
        mock_ip_empty.returncode = 0
        mock_ip_empty.stdout = ""

        # time.time(): deadline=t0+1；循环1 t=0<1 查ip；循环2 t=0<1 查ip；循环3 t=2>1 退出
        mock_time.side_effect = [0, 0, 0, 2]
        mock_run.side_effect = [mock_delete, mock_nmcli, mock_ip_empty, mock_ip_empty]

        wm = WifiManager('wlan0')
        wm.ip_wait_timeout = 1
        wm.ip_wait_interval = 0.1
        success, result = wm.connect('ssid', 'pass')

        self.assertFalse(success)
        self.assertEqual(result, '无法获取IP地址')

    @patch('wifi_manager.subprocess.run')
    def test_connect_uses_arg_list_not_shell(self, mock_run):
        # 验证 nmcli 调用使用参数列表而非 shell=True，避免注入
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0
        mock_ip = MagicMock()
        mock_ip.returncode = 0
        mock_ip.stdout = "inet 192.168.1.1/24 brd 192.168.1.255 scope global wlan0"

        mock_run.side_effect = [mock_delete, mock_nmcli, mock_ip]

        wm = WifiManager('wlan0')
        wm.connect("my'ssid", "pa' ss")

        # 找到 nmcli device wifi connect 那次调用
        connect_calls = [c for c in mock_run.call_args_list
                         if c.args and isinstance(c.args[0], list)
                         and 'wifi' in c.args[0] and 'connect' in c.args[0]]
        self.assertTrue(connect_calls, "应通过参数列表调用 nmcli")
        for c in connect_calls:
            self.assertIsInstance(c.args[0], list)
            self.assertNotIn('shell', c.kwargs)  # 不显式 shell=True

    @patch('wifi_manager.subprocess.run')
    def test_nmcli_wait_is_global_option(self, mock_run):
        # --wait 是 nmcli 全局选项，必须放在子命令 device 之前；
        # 放在末尾会被 nmcli 当作 connect 的额外参数拒绝（实测报错：
        # "无效的额外参数 --wait"）。
        mock_delete = MagicMock()
        mock_delete.returncode = 0
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0
        mock_ip = MagicMock()
        mock_ip.returncode = 0
        mock_ip.stdout = "inet 192.168.1.1/24 brd 192.168.1.255 scope global wlan0"

        mock_run.side_effect = [mock_delete, mock_nmcli, mock_ip]

        wm = WifiManager('wlan0')
        wm.connect("ssid", "pass")

        connect_calls = [c for c in mock_run.call_args_list
                         if c.args and isinstance(c.args[0], list)
                         and 'nmcli' in c.args[0] and 'device' in c.args[0]
                         and 'connect' in c.args[0]]
        self.assertTrue(connect_calls, "应存在 nmcli device wifi connect 调用")
        cmd = connect_calls[0].args[0]
        # nmcli 调用形如 ["nmcli", "--wait", "30", "device", "wifi", "connect", ...]
        self.assertEqual(cmd[1], "--wait", "--wait 必须紧跟 nmcli 作为全局选项")
        self.assertEqual(cmd[2], "30")
        self.assertEqual(cmd[3], "device", "device 子命令应在 --wait 之后")


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
