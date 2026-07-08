"""
AuthPart — 基于 ESP32 eFuse 芯片 ID 的身份识别 Donkeycar Part。

通过 Serial2 (/dev/ttyS6) 与 ESP32 固件的 AuthService 通信，
实现硬件 ID 读取、用户 ID 绑定/解绑/查询。

协议（文本帧，\\n 分隔）：
    请求: CMD:READ_HW_ID / CMD:READ_UID / CMD:WRITE_UID + ARG:<uuid> / CMD:CLEAR_UID
    响应: OK:<data> / ERR:<code>:<desc>
    噪声: BEAT,<ts> / PONG,<seq>,<ts> / ECHO,<text>（Serial2 心跳/ping-pong）

设计文档: docs/plan/ESP32-eFuse-ID-System.md
"""

import logging
import threading
import time

try:
    import serial
except ImportError:
    serial = None

logger = logging.getLogger("AuthPart")


class AuthPart:
    """通过 Serial2 与 ESP32 AuthService 通信的身份识别 Part。

    Donkeycar 生命周期：
        setup()   → 打开串口，读取硬件 ID 和用户 ID
        run()     → 返回当前 token 字典
        shutdown() → 关闭串口
    """

    # ------------------------------------------------------------------
    # 常量
    # ------------------------------------------------------------------
    RETRY_MAX = 3
    RETRY_TIMEOUT_MS = 200
    TWO_LINE_DELAY_S = 0.01

    def __init__(self, port="/dev/ttyS6", baudrate=115200, timeout=0.2):
        self._port = port
        self._baudrate = baudrate
        self._timeout = timeout
        self._lock = threading.Lock()
        self._ser = None
        self._token = {
            "device_hw_id": None,
            "user_id": None,
            "bound": False,
            "signature": None,
            "error": None,
        }

    # ------------------------------------------------------------------
    # Donkeycar Part 生命周期
    # ------------------------------------------------------------------

    def setup(self):
        """打开串口并初始化 token（读取硬件 ID 和用户 ID）。

        串口打开失败或 pyserial 缺失时不抛异常，token 中记录 error。
        """
        if serial is None:
            self._token["error"] = "pyserial not installed"
            logger.error("AuthPart: pyserial 未安装，运行在降级模式")
            return

        try:
            self._ser = serial.Serial(
                self._port, self._baudrate, timeout=self._timeout
            )
            self._ser.reset_input_buffer()
            logger.info("AuthPart: 串口 %s 初始化成功", self._port)
        except Exception as e:
            self._token["error"] = f"serial_open_failed: {e}"
            logger.error("AuthPart: 串口 %s 打开失败: %s", self._port, e)
            return

        # 读取硬件 ID
        hw_id = self._read_hw_id()
        if hw_id:
            self._token["device_hw_id"] = hw_id
            logger.info("AuthPart: 硬件 ID = %s", hw_id)
        else:
            self._token["error"] = "read_hw_id_failed"
            logger.error("AuthPart: 读取硬件 ID 失败")

        # 读取用户 ID
        uid = self._read_uid()
        if uid is not None:
            self._token["user_id"] = uid if uid else None
            self._token["bound"] = bool(uid)
            logger.info("AuthPart: 用户 ID = %s, bound=%s", uid or "(未绑定)",
                        self._token["bound"])
        # uid 为 None（超时）: user_id 保持 None, bound 保持 False

    def run(self):
        """返回当前 token 的浅拷贝。每帧由 Donkeycar 框架调用。"""
        return dict(self._token)

    def shutdown(self):
        """关闭串口（幂等）。"""
        if self._ser and self._ser.is_open:
            try:
                self._ser.close()
                logger.info("AuthPart: 串口已关闭")
            except Exception:
                pass
        self._ser = None

    # ------------------------------------------------------------------
    # 对外接口
    # ------------------------------------------------------------------

    def write_uid(self, uid: str) -> bool:
        """写入用户 ID 到 ESP32 NVS。

        返回 True 表示写入成功，False 表示失败。
        """
        if not self._is_valid_uuid(uid):
            logger.error("AuthPart: UUID 格式无效: %s", uid)
            return False

        if self._ser is None:
            logger.error("AuthPart: 串口未就绪")
            return False

        resp = self._send_two_line_cmd("WRITE_UID", uid)
        if resp and resp.startswith("OK:written"):
            self._token["user_id"] = uid
            self._token["bound"] = True
            logger.info("AuthPart: write_uid 成功")
            return True

        logger.error("AuthPart: write_uid 失败, resp=%s", resp)
        return False

    def clear_uid(self) -> bool:
        """清空 ESP32 NVS 中的用户 ID。

        返回 True 表示清空成功，False 表示失败。
        """
        if self._ser is None:
            logger.error("AuthPart: 串口未就绪")
            return False

        resp = self._send_cmd("CLEAR_UID")
        if resp and resp.startswith("OK:cleared"):
            self._token["user_id"] = None
            self._token["bound"] = False
            logger.info("AuthPart: clear_uid 成功")
            return True

        logger.error("AuthPart: clear_uid 失败, resp=%s", resp)
        return False

    # ------------------------------------------------------------------
    # 内部命令发送
    # ------------------------------------------------------------------

    def _read_hw_id(self):
        """发送 CMD:READ_HW_ID 并返回硬件 ID 字符串，失败返回 None。"""
        resp = self._send_cmd("READ_HW_ID")
        if resp and resp.startswith("OK:") and len(resp) > 3:
            return resp[3:]  # 去掉 "OK:" 前缀
        return None

    def _read_uid(self):
        """发送 CMD:READ_UID 并返回用户 ID 字符串。
        返回 "" 表示未绑定，返回 None 表示通信失败。
        """
        resp = self._send_cmd("READ_UID")
        if resp and resp.startswith("OK:"):
            return resp[3:]  # 未绑定时为 ""
        return None

    def _send_cmd(self, cmd):
        """发送单行命令 CMD:<cmd>\\n，带重试。

        最多重试 RETRY_MAX 次，每次超时 RETRY_TIMEOUT_MS 毫秒。
        返回匹配到的响应行（OK:... 或 ERR:...），全部失败返回 None。
        """
        with self._lock:
            for attempt in range(self.RETRY_MAX):
                try:
                    self._ser.write(f"CMD:{cmd}\n".encode("utf-8"))
                    self._ser.flush()
                except Exception as e:
                    logger.error("AuthPart: 串口写入失败: %s", e)
                    return None

                resp = self._wait_response(self.RETRY_TIMEOUT_MS)
                if resp is not None:
                    return resp

                logger.warning(
                    "AuthPart: CMD:%s 第 %d/%d 次超时",
                    cmd, attempt + 1, self.RETRY_MAX
                )

            logger.error("AuthPart: CMD:%s 全部 %d 次重试耗尽", cmd, self.RETRY_MAX)
            return None

    def _send_two_line_cmd(self, cmd, arg):
        """发送两行命令：CMD:<cmd>\\n + ARG:<arg>\\n（用于 WRITE_UID）。

        两行协议不重试（因为状态机在 CMD 发送后已进入 AUTH_WAIT_ARG）。
        返回匹配到的响应行，失败返回 None。
        """
        with self._lock:
            try:
                self._ser.write(f"CMD:{cmd}\n".encode("utf-8"))
                self._ser.flush()
                time.sleep(self.TWO_LINE_DELAY_S)
                self._ser.write(f"ARG:{arg}\n".encode("utf-8"))
                self._ser.flush()
            except Exception as e:
                logger.error("AuthPart: 两行命令串口写入失败: %s", e)
                return None

            return self._wait_response(self.RETRY_TIMEOUT_MS)

    # ------------------------------------------------------------------
    # 响应等待与噪声过滤
    # ------------------------------------------------------------------

    def _wait_response(self, timeout_ms):
        """在超时时间内轮询串口，过滤噪声行，返回第一条 OK: 或 ERR: 行。

        被过滤的噪声：
            - 空行 / decode 失败
            - BEAT,<ts>（Serial2 心跳）
            - PONG,<seq>,<ts>（ping-pong 应答）
            - ECHO,<text>（ping-pong 回显）

        返回匹配到的行（含前缀），超时返回 None。
        """
        deadline = time.monotonic() + timeout_ms / 1000.0

        while time.monotonic() < deadline:
            try:
                raw = self._ser.readline()
            except Exception as e:
                logger.error("AuthPart: 串口读取异常: %s", e)
                return None

            if not raw:
                time.sleep(0.001)
                continue

            try:
                line = raw.decode("utf-8", errors="ignore").strip()
            except Exception:
                continue

            if not line:
                continue

            # 噪声过滤
            if line.startswith("BEAT,"):
                logger.debug("AuthPart: 过滤 BEAT: %s", line)
                continue
            if line.startswith("PONG,"):
                logger.debug("AuthPart: 过滤 PONG: %s", line)
                continue
            if line.startswith("ECHO,"):
                logger.debug("AuthPart: 过滤 ECHO: %s", line)
                continue

            # 有效响应
            if line.startswith("OK:") or line.startswith("ERR:"):
                logger.debug("AuthPart: RX %s", line)
                return line

            # 未识别的行：记录并继续等待
            logger.debug("AuthPart: 忽略未知行: %s", line)

        return None

    # ------------------------------------------------------------------
    # UUID 格式校验（静态方法，方便测试）
    # ------------------------------------------------------------------

    @staticmethod
    def _is_valid_uuid(s: str) -> bool:
        """判断字符串是否为有效 UUID v4 格式（36 字符，含 4 个连字符）。"""
        if len(s) != 36:
            return False
        for i, c in enumerate(s):
            if i in (8, 13, 18, 23):
                if c != "-":
                    return False
            else:
                if not c.isalnum():  # 接受大小写十六进制
                    return False
                # 进一步检查是否为十六进制字符
                if not (("0" <= c <= "9") or ("a" <= c <= "f") or ("A" <= c <= "F")):
                    return False
        return True
