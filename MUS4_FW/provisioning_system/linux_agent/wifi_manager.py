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
        """连接目标WiFi。采用两步式 profile 创建，规避 nmcli 1.54 的 key-mgmt 推断缺陷。"""
        self.logger.info(f"正在连接WiFi: {ssid}")

        # 1. 删除可能存在的旧 profile：残留 profile 的 key-mgmt/psk 字段可能损坏，
        #    复用会导致 "key-mgmt: 缺少属性" 或 "需要密钥，但未提供"。
        subprocess.run(
            ["nmcli", "connection", "delete", ssid],
            capture_output=True, text=True)

        # 2. 显式创建 profile：nmcli 1.54 的 `device wifi connect <ssid> password <pwd>`
        #    不会自动推断 key-mgmt（实测报 key-mgmt 缺失），必须用 connection add 显式
        #    指定 wifi-sec.key-mgmt wpa-psk。参数列表形式避免 shell 注入。
        res = subprocess.run(
            ["nmcli", "connection", "add", "type", "wifi",
             "ifname", self.interface, "con-name", ssid, "ssid", ssid,
             "wifi-sec.key-mgmt", "wpa-psk", "wifi-sec.psk", password],
            capture_output=True, text=True)

        if res.returncode != 0:
            self.logger.error(f"创建连接配置失败: {res.stderr}")
            return False, "创建连接配置失败"

        # 3. 激活连接（--wait 是 nmcli 全局选项，必须放在子命令 connection 之前）
        res = subprocess.run(
            ["nmcli", "--wait", "30", "connection", "up", ssid],
            capture_output=True, text=True)

        if res.returncode != 0:
            self.logger.error(f"WiFi连接失败: {res.stderr}")
            return False, "连接失败或超时"

        # 4. 轮询等待 DHCP 分配 IP（nmcli 返回成功时 IP 可能尚未就绪）
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
