import unittest
from unittest.mock import patch, MagicMock
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../linux_agent')))

from wifi_manager import WifiManager
from serial_comm import SerialComm
from agent import ProvisioningAgent

class TestWifiManager(unittest.TestCase):
    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_success(self, mock_run):
        # 模拟 nmcli 成功 和 ip addr 成功
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 0
        
        mock_ip = MagicMock()
        mock_ip.returncode = 0
        mock_ip.stdout = "inet 192.168.1.100/24 brd 192.168.1.255 scope global dynamic wlan0"
        
        mock_run.side_effect = [MagicMock(), mock_nmcli, mock_ip]
        
        wm = WifiManager('wlan0')
        success, result = wm.connect('newhome_iot', 'wxl922922')
        
        self.assertTrue(success)
        self.assertEqual(result, '192.168.1.100')

    @patch('wifi_manager.subprocess.run')
    def test_wifi_connect_failure(self, mock_run):
        # 模拟 nmcli 失败 (如密码错误)
        mock_nmcli = MagicMock()
        mock_nmcli.returncode = 1
        mock_nmcli.stderr = "Error: Connection activation failed."
        
        mock_run.side_effect = [MagicMock(), mock_nmcli]
        
        wm = WifiManager('wlan0')
        success, result = wm.connect('wrong_ssid', 'wrong_pass')
        
        self.assertFalse(success)
        self.assertEqual(result, '连接失败或超时')


class TestProvisioningAgentE2E(unittest.TestCase):
    @patch('agent.WifiManager')
    @patch('agent.SerialComm')
    def test_end_to_end_success_flow(self, MockSerialComm, MockWifiManager):
        # 初始化 Mock 对象
        mock_serial = MockSerialComm.return_value
        mock_wifi = MockWifiManager.return_value
        
        # 模拟串口收到正确的配网字符串
        mock_serial.read_line.side_effect = ["WIFI|newhome_iot|wxl922922", ""]
        # 模拟WiFi连接成功并返回IP
        mock_wifi.connect.return_value = (True, "192.168.1.150")
        
        agent = ProvisioningAgent()
        # 执行单次逻辑测试 (非循环)
        agent.handle_provisioning_request("WIFI|newhome_iot|wxl922922")
        
        # 验证调用链：是否断开旧热点
        mock_wifi.disconnect_ap.assert_called_once()
        # 验证调用链：是否发起了连接
        mock_wifi.connect.assert_called_with("newhome_iot", "wxl922922")
        # 验证调用链：串口是否回传了正确的信息
        mock_serial.write_line.assert_any_call("STATUS|CONNECTING")
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

if __name__ == '__main__':
    unittest.main(verbosity=2)
