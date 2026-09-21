# -*- coding: utf-8 -*-
"""ZCode 远控链接（WebConsoleAssets.h 内嵌 JS）node 行为测试的 pytest 包装。

行为测试本体在同目录 `zcode_remote_url.test.mjs`：从 .h 提取真实函数实体
放进 node:vm 沙箱跑断言（normalize 边界矩阵 + 单击/双击交互流），与 DD 侧
EnterButtons.tsx 的 normalizeRemoteUrl 语义逐条对齐。本机无 node 时 skip。
"""

import shutil
import subprocess
from pathlib import Path

import pytest

TEST_JS = Path(__file__).with_name("zcode_remote_url.test.mjs")


def test_zcode_remote_url_node_behavior():
    node = shutil.which("node")
    if node is None:
        pytest.skip("本机无 node，跳过固件内嵌 JS 行为测试")
    proc = subprocess.run(
        [node, str(TEST_JS)], capture_output=True, text=True, timeout=60
    )
    assert proc.returncode == 0, (
        f"ZCode 远控链接 node 行为测试失败：\n{proc.stdout}\n{proc.stderr}"
    )
