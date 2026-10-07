#!/usr/bin/env bash
# 主控板编译矩阵：板 A / 板 B 各编一次，产物带板型后缀。
# 档案定义与选择优先级见 libraries/mus4_core/src/BoardProfile.h
# 用法：./tools/build_profiles.sh
set -euo pipefail
cd "$(dirname "$0")/.."

echo "==> 板 A（当前主控板，默认）"
python3 arduino-cli.py -c --sketch MUS4_FW.ino --build-path build/boardA --bin-tag boardA

echo "==> 板 B（另一块主控板）"
python3 arduino-cli.py -c --sketch MUS4_FW.ino -D MUS4_BOARD_B --build-path build/boardB --bin-tag boardB

echo "编译矩阵完成："
echo "  build/boardA/MUS4_FW_boardA.bin"
echo "  build/boardB/MUS4_FW_boardB.bin"
