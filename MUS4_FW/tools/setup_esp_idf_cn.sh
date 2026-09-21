#!/usr/bin/env bash
# setup_esp_idf_cn.sh — 在中国大陆网络下安装 ESP-IDF 并验证编译/下载
# 用法:
#   bash tools/setup_esp_idf_cn.sh
# 可选环境变量:
#   IDF_VERSION     默认 v5.4.4
#   IDF_DIR         默认 $HOME/esp/esp-idf
#   IDF_TOOLS_PATH  默认 $HOME/.espressif
set -e

IDF_VERSION="$IDF_VERSION"
[ -z "$IDF_VERSION" ] && IDF_VERSION="v5.4.4"
IDF_DIR="$IDF_DIR"
[ -z "$IDF_DIR" ] && IDF_DIR="$HOME/esp/esp-idf"
IDF_TOOLS_PATH="$IDF_TOOLS_PATH"
[ -z "$IDF_TOOLS_PATH" ] && IDF_TOOLS_PATH="$HOME/.espressif"
PIP_INDEX_URL="$PIP_INDEX_URL"
[ -z "$PIP_INDEX_URL" ] && PIP_INDEX_URL="https://pypi.tuna.tsinghua.edu.cn/simple"

export IDF_TOOLS_PATH
export PIP_INDEX_URL
# 关键：把 github.com 下载重定向到乐鑫镜像，否则 install.sh 会卡在 GitHub
export IDF_GITHUB_ASSETS="dl.espressif.com/github_assets"

echo "== 1/5 apt 依赖 =="
sudo apt-get update || true
sudo apt-get install -y git wget flex bison gperf python3-pip python3-venv  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

echo "== 2/5 获取 ESP-IDF $IDF_VERSION =="
mkdir -p "$(dirname "$IDF_DIR")"
if [ ! -d "$IDF_DIR/.git" ]; then
  git clone --depth 1 --branch "$IDF_VERSION" https://jihulab.com/esp-mirror/espressif/esp-idf.git "$IDF_DIR"
fi
cd "$IDF_DIR"
# 非递归：跳过指向 github 的 cmock 测试子模块 vendor/c_exception
git submodule update --init --depth 1 --jobs 4

echo "== 3/5 安装工具链（esp32）=="
PATH=/usr/bin:$PATH ./install.sh esp32

echo "== 4/5 激活方式 =="
echo "  以后运行: . $IDF_DIR/export.sh"
echo "  或加入 ~/.bashrc: alias get_idf='. $HOME/esp/esp-idf/export.sh'"

echo "== 5/5 编译 hello_world 验证 =="
# 确保 export.sh 选到系统 python3（PATH 里若先有别的 venv 会报 idf5.4_py3.x_env not found）
export PATH=/usr/bin:$PATH
cd "$IDF_DIR/examples/get-started/hello_world"
. "$IDF_DIR/export.sh"
idf.py set-target esp32
idf.py build

echo "OK. 烧录: idf.py -p /dev/ttyACM1 flash monitor"
