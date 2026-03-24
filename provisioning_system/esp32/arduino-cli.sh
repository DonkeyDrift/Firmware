#!/bin/bash
# 简单的 Arduino-CLI 编译与烧录脚本封装
FQBN="esp32:esp32:esp32"
PORT="/dev/ttyUSB0"
SKETCH="esp32_wifi_provisioning/esp32_wifi_provisioning.ino"

echo "=== Arduino-CLI 编译流程 ==="
arduino-cli compile --fqbn $FQBN $SKETCH

if [ $? -eq 0 ]; then
    echo "编译成功！"
    echo "使用以下命令烧录: arduino-cli upload -p $PORT --fqbn $FQBN $SKETCH"
else
    echo "编译失败，请检查环境依赖。"
fi
