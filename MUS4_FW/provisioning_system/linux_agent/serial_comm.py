import logging

try:
    import serial
except ImportError:
    serial = None

class SerialComm:
    """处理ESP32与Linux间的串口通信"""
    # Serial2 (ESP32 GPIO19/18) 物理对接到 Linux /dev/ttyS6（抓包实测确认）。
    # /dev/ttyS4 对接的是 Serial1 遥测通道，不可用。
    def __init__(self, port='/dev/ttyS6', baudrate=115200, timeout=1):
        self.logger = logging.getLogger("SerialComm")
        if serial is None:
            self.logger.warning("未安装pyserial，正在运行于Mock模式")
            self.ser = None
            return
            
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
            self.logger.info(f"串口 {port} 初始化成功")
        except Exception as e:
            self.logger.error(f"串口初始化失败: {e}")
            self.ser = None

    def read_line(self):
        """非阻塞读取一行数据"""
        if self.ser and self.ser.in_waiting > 0:
            try:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    self.logger.debug(f"RX: {line}")
                return line
            except Exception as e:
                self.logger.error(f"串口读取异常: {e}")
        return ""

    def write_line(self, data):
        """发送一行数据"""
        if self.ser:
            try:
                self.logger.debug(f"TX: {data}")
                self.ser.write((data + '\n').encode('utf-8'))
                self.ser.flush()
                return True
            except Exception as e:
                self.logger.error(f"串口发送异常: {e}")
        return False
