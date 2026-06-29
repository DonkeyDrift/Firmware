"""ESP32 eFuse 芯片 ID 身份识别系统 — AuthPart 单元测试。

测试覆盖：
- READ_HW_ID / READ_UID / WRITE_UID / CLEAR_UID 四条命令的正常流
- 各错误码的解析（01-04）
- 多行协议 WRITE_UID（CMD + ARG 分两行发送）
- 超时 + 3 次重试机制
- 串口打开失败的优雅降级
- token 字典输出格式
- 线程安全（threading.Lock 保护 _send_cmd）
"""

import importlib.util
import pathlib
import threading
import unittest
from unittest.mock import MagicMock, patch


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "parts" / "auth_part.py"

# 测试时动态加载 auth_part 模块
SPEC = importlib.util.spec_from_file_location("auth_part", MODULE_PATH)
AUTH = importlib.util.module_from_spec(SPEC)


# ---------------------------------------------------------------------------
# 辅助函数：创建模拟串口对象，可预设 readline 返回序列
# ---------------------------------------------------------------------------
def _make_mock_serial(readline_sequence, write_side_effect=None):
    """构建模拟 serial.Serial 实例。

    readline_sequence: list[str] — 每次调用 readline() 依次返回的值。
    write_side_effect: callable | None — 可选，用于验证写入内容。
    """
    mock = MagicMock()
    mock.readline.side_effect = readline_sequence
    if write_side_effect:
        mock.write.side_effect = write_side_effect
    mock.in_waiting = 0
    mock.is_open = True
    return mock


# ---------------------------------------------------------------------------
# AuthPart 单元测试
# ---------------------------------------------------------------------------
class TestAuthPartCommands(unittest.TestCase):
    """覆盖四条 Auth 命令的正常流和错误码解析。"""

    def setUp(self):
        """每个测试用例前重置 mock。"""
        self.mock_ser = None
        self.part = None

    def _create_part_with_mock(self, readline_sequence):
        """用模拟串口创建 AuthPart 实例并执行 setup()。"""
        self.mock_ser = _make_mock_serial(readline_sequence)
        with patch("serial.Serial", return_value=self.mock_ser):
            self.part = AUTH.AuthPart(port="/dev/fake", baudrate=115200, timeout=0.2)
            self.part.setup()

    # ---- READ_HW_ID ----

    def test_read_hw_id_returns_chip_id(self):
        """READ_HW_ID 应返回 12 字符小写 hex 硬件 ID。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",   # READ_HW_ID 响应
            "OK:\n",                 # READ_UID 响应（未绑定）
        ])
        token = self.part.run()
        self.assertEqual(token["device_hw_id"], "a1b2c3d4e5f6")
        self.assertFalse(token["bound"])

    def test_read_hw_id_nack_then_ok_on_retry(self):
        """READ_HW_ID 首次超时空行、第二次返回 OK。"""
        self._create_part_with_mock([
            "",                      # 超时（readline 返回空字符串）
            "OK:abcdef123456\n",     # 重试成功
            "OK:\n",                 # READ_UID
        ])
        token = self.part.run()
        self.assertEqual(token["device_hw_id"], "abcdef123456")
        # 验证写入次数：第 1 次 + 第 2 次（共 2 次 CMD:READ_HW_ID）
        write_calls = [c[0][0] for c in self.mock_ser.write.call_args_list]
        hw_id_calls = [c for c in write_calls if b"READ_HW_ID" in c]
        self.assertEqual(len(hw_id_calls), 2)

    # ---- READ_UID ----

    def test_read_uid_when_bound_returns_uuid(self):
        """READ_UID 已绑定时应返回 UUID 且 bound=True。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:550e8400-e29b-41d4-a716-446655440000\n",
        ])
        token = self.part.run()
        self.assertEqual(token["user_id"], "550e8400-e29b-41d4-a716-446655440000")
        self.assertTrue(token["bound"])

    def test_read_uid_when_not_bound_returns_empty(self):
        """READ_UID 未绑定时 OK 后无数据，user_id 应为 None。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",                 # 空 OK，未绑定
        ])
        token = self.part.run()
        self.assertIsNone(token["user_id"])
        self.assertFalse(token["bound"])

    # ---- WRITE_UID ----

    def test_write_uid_success(self):
        """WRITE_UID 成功应返回 True，且更新 token 的 user_id。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        # 为 write_uid 准备新的 readline 序列
        self.mock_ser.readline.side_effect = [
            "",                      # CMD:WRITE_UID 的 readline（等 ARG 回复？不，看协议）
        ]
        # 修正：WRITE_UID 协议是两行发送（CMD + ARG），然后读取一行 OK
        # 需要用不同的 mock 设置方式
        # 这里直接验证方法签名和行为
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = ["OK:written\n"]
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part.write_uid("550e8400-e29b-41d4-a716-446655440000")
        self.assertTrue(result)

        # 验证发送了 CMD:WRITE_UID 和 ARG:<uuid>
        write_calls = [c[0][0].decode("utf-8", errors="ignore") for c in self.mock_ser.write.call_args_list]
        self.assertIn("CMD:WRITE_UID\n", write_calls)
        self.assertIn("ARG:550e8400-e29b-41d4-a716-446655440000\n", write_calls)

    def test_write_uid_nvs_write_fail(self):
        """WRITE_UID 返回 ERR:03 时 write_uid 应返回 False。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = ["ERR:03:NVS write fail\n"] * 3
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part.write_uid("550e8400-e29b-41d4-a716-446655440000")
        self.assertFalse(result)
        # 应重试 3 次
        write_calls = [c[0][0] for c in self.mock_ser.write.call_args_list]
        cmd_count = sum(1 for c in write_calls if b"WRITE_UID" in c)
        self.assertEqual(cmd_count, 3)

    # ---- CLEAR_UID ----

    def test_clear_uid_success(self):
        """CLEAR_UID 成功应返回 True，且更新 token 的 user_id 为 None。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:550e8400-e29b-41d4-a716-446655440000\n",
        ])
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = ["OK:cleared\n"]
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part.clear_uid()
        self.assertTrue(result)

        # 验证发送了 CMD:CLEAR_UID
        write_calls = [c[0][0].decode("utf-8", errors="ignore") for c in self.mock_ser.write.call_args_list]
        self.assertIn("CMD:CLEAR_UID\n", write_calls)

    # ---- 未知命令 ----

    def test_unknown_command_returns_err(self):
        """发送未知命令时 ESP32 返回 ERR:01，_send_cmd 应返回 None。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = ["ERR:01:unknown command\n"] * 3
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part._send_cmd("CMD:FOO\n")
        self.assertIsNone(result)

    # ---- 超时重试 ----

    def test_timeout_with_retry_exhausted(self):
        """3 次全部超时后 _send_cmd 应返回 None。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = [""] * 3
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part._send_cmd("CMD:READ_HW_ID\n")
        self.assertIsNone(result)
        # 验证尝试了 3 次
        write_calls = [c[0][0] for c in self.mock_ser.write.call_args_list]
        self.assertEqual(len(write_calls), 3)

    def test_timeout_succeeds_on_second_retry(self):
        """首次超时、第二次返回 OK，_send_cmd 应成功。"""
        self._create_part_with_mock([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        self.mock_ser.reset_mock()
        self.mock_ser.readline.side_effect = ["", "OK:data\n"]
        self.mock_ser.in_waiting = 0
        self.mock_ser.is_open = True

        result = self.part._send_cmd("CMD:READ_UID\n")
        self.assertEqual(result, "OK:data")
        # 验证尝试了 2 次
        write_calls = [c[0][0] for c in self.mock_ser.write.call_args_list]
        self.assertEqual(len(write_calls), 2)


class TestAuthPartLifecycle(unittest.TestCase):
    """覆盖 AuthPart 生命周期和错误处理。"""

    def test_setup_serial_open_failed(self):
        """串口打开失败时 token 应包含 error 字段，不抛异常。"""
        with patch("serial.Serial", side_effect=OSError("Permission denied")):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()
            token = part.run()
            self.assertIn("error", token)
            self.assertIn("serial_open_failed", token["error"])
            self.assertIsNone(token["device_hw_id"])
            self.assertIsNone(token["user_id"])
            self.assertFalse(token["bound"])

    def test_shutdown_closes_serial(self):
        """shutdown() 应关闭串口。"""
        mock_ser = _make_mock_serial(["OK:abcdef123456\n", "OK:\n"])
        with patch("serial.Serial", return_value=mock_ser):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()
            part.shutdown()
            mock_ser.close.assert_called_once()

    def test_shutdown_when_serial_is_none(self):
        """串口打开失败后 shutdown() 不抛异常。"""
        with patch("serial.Serial", side_effect=OSError("No such device")):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()
            # 不应抛异常
            part.shutdown()


class TestAuthPartTokenFormat(unittest.TestCase):
    """验证 token 输出格式符合规范。"""

    def test_token_structure_when_bound(self):
        """已绑定时 token 应包含完整字段。"""
        mock_ser = _make_mock_serial([
            "OK:a1b2c3d4e5f6\n",
            "OK:550e8400-e29b-41d4-a716-446655440000\n",
        ])
        with patch("serial.Serial", return_value=mock_ser):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()
            token = part.run()

            self.assertIn("device_hw_id", token)
            self.assertIn("user_id", token)
            self.assertIn("bound", token)
            self.assertIn("signature", token)
            self.assertEqual(token["device_hw_id"], "a1b2c3d4e5f6")
            self.assertEqual(token["user_id"], "550e8400-e29b-41d4-a716-446655440000")
            self.assertTrue(token["bound"])
            self.assertIsNone(token["signature"])

    def test_token_structure_when_unbound(self):
        """未绑定时 bound=False, user_id=None。"""
        mock_ser = _make_mock_serial([
            "OK:abcdef123456\n",
            "OK:\n",
        ])
        with patch("serial.Serial", return_value=mock_ser):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()
            token = part.run()

            self.assertEqual(token["device_hw_id"], "abcdef123456")
            self.assertIsNone(token["user_id"])
            self.assertFalse(token["bound"])


class TestAuthPartThreadSafety(unittest.TestCase):
    """验证 threading.Lock 保护串口操作。"""

    def test_concurrent_write_uid_serialized(self):
        """并发调用 write_uid 应串行执行，不出现数据竞争。"""
        mock_ser = _make_mock_serial([
            "OK:a1b2c3d4e5f6\n",
            "OK:\n",
        ])
        with patch("serial.Serial", return_value=mock_ser):
            part = AUTH.AuthPart(port="/dev/fake")
            part.setup()

        # 模拟慢速响应
        call_order = []
        original_write = mock_ser.write

        def tracking_write(data):
            call_order.append(data)
            return original_write(data)

        mock_ser.write.side_effect = tracking_write
        mock_ser.readline.side_effect = ["OK:written\n"] * 10
        mock_ser.reset_mock()

        results = []

        def do_write(uid_suffix):
            results.append(
                part.write_uid(f"550e8400-e29b-41d4-a716-4466554400{uid_suffix:02d}")
            )

        threads = [
            threading.Thread(target=do_write, args=(i,))
            for i in range(5)
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        # 全部应成功
        self.assertTrue(all(results))
        # 每个 write_uid 发送 2 次写入（CMD + ARG），5 个线程 = 10 次写入
        self.assertEqual(len(call_order), 10)


class TestAuthPartDefaultConfig(unittest.TestCase):
    """验证默认配置值。"""

    def test_default_port_and_baudrate(self):
        """默认端口和波特率与 spec 一致。"""
        mock_ser = _make_mock_serial(["OK:abcdef123456\n", "OK:\n"])
        with patch("serial.Serial", return_value=mock_ser) as mock_serial_cls:
            part = AUTH.AuthPart()
            part.setup()
            mock_serial_cls.assert_called_once()
            call_kwargs = mock_serial_cls.call_args.kwargs
            self.assertEqual(call_kwargs["port"], "/dev/ttyS6")
            self.assertEqual(call_kwargs["baudrate"], 115200)
            self.assertAlmostEqual(call_kwargs["timeout"], 0.2)


if __name__ == "__main__":
    unittest.main()
