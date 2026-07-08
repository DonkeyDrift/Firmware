import argparse
import time
import logging
from wifi_manager import WifiManager
from serial_comm import SerialComm

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Serial2 (ESP32 GPIO19/18) 物理对接到 Linux /dev/ttyS6（2026-07-01 抓包实测确认）。
# 注意：/dev/ttyS4 对接的是 Serial1 遥测通道，不是配网通道。
DEFAULT_PORT = "/dev/ttyS6"
DEFAULT_INTERFACE = "wlp1s0"


class ProvisioningAgent:
    """Linux端配网代理守护进程"""
    def __init__(self, interface=DEFAULT_INTERFACE, port=DEFAULT_PORT):
        self.wifi_manager = WifiManager(interface)
        self.serial_comm = SerialComm(port)
        self.running = False

    def start(self):
        self.running = True
        logging.info("配网代理服务已启动，正在监听串口...")

        while self.running:
            line = self.serial_comm.read_line()
            if line.startswith("WIFI|"):
                self.handle_provisioning_request(line)
            time.sleep(0.1)

    def handle_provisioning_request(self, data):
        """解析并处理配网指令。帧格式 WIFI|<ssid>|<password>，密码可含 |。"""
        # split("|", 2) 限制分割 2 次：密码中的 | 不被截断
        parts = data.split("|", 2)
        if len(parts) < 3:
            logging.warning(f"配网帧格式非法: {data}")
            self.serial_comm.write_line("FAIL|配网帧格式非法")
            return
        ssid = parts[1]
        password = parts[2]

        logging.info(f"收到配网请求: SSID={ssid}")
        # 状态用小写 connecting，与 ESP32 Web 前端状态映射表一致
        self.serial_comm.write_line("STATUS|connecting")

        # 断开当前AP连接，释放网卡
        self.wifi_manager.disconnect_ap()
        time.sleep(1)  # 给网卡一点切换状态的时间

        # 连接新WiFi
        success, result = self.wifi_manager.connect(ssid, password)

        if success:
            logging.info(f"配网成功，IP: {result}")
            self.serial_comm.write_line(f"OK|{result}")
        else:
            logging.error(f"配网失败: {result}")
            self.serial_comm.write_line(f"FAIL|{result}")


def parse_args():
    parser = argparse.ArgumentParser(description="MUS4 Linux 端配网代理守护进程")
    parser.add_argument("--port", default=DEFAULT_PORT,
                        help=f"串口设备路径（默认 {DEFAULT_PORT}，即 ESP32 Serial2 对接端口）")
    parser.add_argument("--interface", default=DEFAULT_INTERFACE,
                        help=f"无线网卡接口名（默认 {DEFAULT_INTERFACE}）")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    agent = ProvisioningAgent(interface=args.interface, port=args.port)
    agent.start()
