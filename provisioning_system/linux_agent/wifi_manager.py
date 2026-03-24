import subprocess
import re
import logging

class WifiManager:
    """处理Linux主机的WiFi连接操作"""
    def __init__(self, interface="wlan0"):
        self.interface = interface
        self.logger = logging.getLogger("WifiManager")

    def disconnect_ap(self):
        """断开当前的热点连接"""
        cmd = f"nmcli device disconnect {self.interface}"
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return res.returncode == 0

    def connect(self, ssid, password):
        """连接目标WiFi"""
        self.logger.info(f"正在连接WiFi: {ssid}")
        
        # 1. 删除可能存在的旧配置
        subprocess.run(f"nmcli connection delete '{ssid}'", shell=True, capture_output=True)
        
        # 2. 尝试连接新网络
        cmd = f"nmcli device wifi connect '{ssid}' password '{password}' ifname {self.interface}"
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        if res.returncode != 0:
            self.logger.error(f"WiFi连接失败: {res.stderr}")
            return False, "连接失败或超时"
            
        # 3. 获取IP地址
        return self._get_ip_address()

    def _get_ip_address(self):
        """解析分配到的IP地址"""
        cmd = f"ip -4 addr show {self.interface}"
        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        if res.returncode == 0:
            match = re.search(r'inet\s+(\d+\.\d+\.\d+\.\d+)', res.stdout)
            if match:
                ip = match.group(1)
                self.logger.info(f"获取到IP地址: {ip}")
                return True, ip
                
        return False, "无法获取IP地址"
