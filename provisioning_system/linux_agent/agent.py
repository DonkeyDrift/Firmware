import time
import logging
from wifi_manager import WifiManager
from serial_comm import SerialComm

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class ProvisioningAgent:
    """Linux端配网代理守护进程"""
    def __init__(self, interface="wlan0", port="/dev/ttyS4"):
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
        """解析并处理配网指令"""
        parts = data.split("|")
        if len(parts) >= 3:
            ssid = parts[1]
            password = parts[2]
            
            logging.info(f"收到配网请求: SSID={ssid}")
            self.serial_comm.write_line("STATUS|CONNECTING")
            
            # 断开当前AP连接，释放网卡
            self.wifi_manager.disconnect_ap()
            time.sleep(1) # 给网卡一点切换状态的时间
            
            # 连接新WiFi
            success, result = self.wifi_manager.connect(ssid, password)
            
            if success:
                logging.info(f"配网成功，IP: {result}")
                self.serial_comm.write_line(f"OK|{result}")
            else:
                logging.error(f"配网失败: {result}")
                self.serial_comm.write_line(f"FAIL|{result}")

if __name__ == "__main__":
    agent = ProvisioningAgent()
    agent.start()
