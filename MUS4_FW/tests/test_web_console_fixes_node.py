# -*- coding: utf-8 -*-
"""Drifter Console 前端修复（WebConsoleAssets.h 内嵌 JS）node 行为测试的 pytest 包装。

行为测试本体在同目录 `web_console_fixes.test.mjs`：从 .h 提取真实函数实体
放进 node:vm 沙箱跑断言（seq 回退重置 / 命令错误映射矩阵 / STA 配网占位清理 /
handoff modal 标记 / apply_pending 等待 / tub 批量帧录制 / 校准弹窗 live 行）。
本机无 node 时 skip。
"""

import shutil
import subprocess
from pathlib import Path

import pytest

TEST_JS = Path(__file__).with_name("web_console_fixes.test.mjs")


def test_web_console_fixes_node_behavior():
    node = shutil.which("node")
    if node is None:
        pytest.skip("本机无 node，跳过固件内嵌 JS 行为测试")
    proc = subprocess.run(
        [node, str(TEST_JS)], capture_output=True, text=True, timeout=60
    )
    assert proc.returncode == 0, (
        f"Drifter Console 前端修复 node 行为测试失败：\n{proc.stdout}\n{proc.stderr}"
    )
