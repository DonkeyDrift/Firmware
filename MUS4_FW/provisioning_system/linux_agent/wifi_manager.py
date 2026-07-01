import subprocess
import re
import time
import logging


class WifiManager:
    """处理Linux主机的WiFi连接操作"""
    def __init__(self, interface="wlp1s0"):
        self.interface = interface
        self.logger = logging.getLogger("WifiManager")
        # 等待 DHCP 分配 IP 的超时与轮询间隔（nmcli connect 成功不代表 IP 已就绪）
        self.ip_wait_timeout = 10
        self.ip_wait_interval = 0.5

    def disconnect_ap(self):
        """断开当前的热点连接"""
        res = subprocess.run(
            ["nmcli", "device", "disconnect", self.interface],
            capture_output=True, text=True)
        return res.returncode == 0

    def connect(self, ssid, password):
        """连接目标WiFi。使用参数列表调用 nmcli，避免 shell 注入。"""
        self.logger.info(f"正在连接WiFi: {ssid}")

        # 1. 删除可能存在的旧配置
        subprocess.run(
            ["nmcli", "connection", "delete", ssid],
            capture_output=True, text=True)

        # 2. 尝试连接新网络（--wait 30 显式控制 nmcli 自身等待时长）
        res = subprocess.run(
            ["nmcli", "device", "wifi", "connect", ssid,
             "password", password, "ifname", self.interface, "--wait", "30"],
            capture_output=True, text=True)

        if res.returncode != 0:
            self.logger.error(f"WiFi连接失败: {res.stderr}")
            return False, "连接失败或超时"

        # 3. 轮询等待 DHCP 分配 IP（nmcli 返回成功时 IP 可能尚未就绪）
        return self._wait_for_ip_address()

    def _get_ip_address(self):
        """查询一次网卡 IPv4 地址。返回 (success, ip_or_reason)。"""
        res = subprocess.run(
            ["ip", "-4", "addr", "show", self.interface],
            capture_output=True, text=True)

        if res.returncode == 0:
            match = re.search(r'inet\s+(\d+\.\d+\.\d+\.\d+)', res.stdout)
            if match:
                ip = match.group(1)
                self.logger.info(f"获取到IP地址: {ip}")
                return True, ip

        return False, None

    def _wait_for_ip_address(self):
        """轮询等待 IP 就绪，避免 DHCP 未分配时误报失败。"""
        deadline = time.time() + self.ip_wait_timeout
        while time.time() < deadline:
            ok, ip = self._get_ip_address()
            if ok:
                return True, ip
            time.sleep(self.ip_wait_interval)
        return False, "无法获取IP地址"
