# MUS4_FW IDF 工程骨架（阶段 A 参考）

这是本机（aarch64 Ubuntu 24.04 / ESP-IDF v5.4.4 + arduino-esp32 3.3.11）**实测可编译**的
"Arduino as an ESP-IDF component" 最小工程，用来演示如何把现有 `MUS4_FW.ino` 迁到 `idf.py` 构建。

完整分析、API 映射表与中国大陆网络下的环境搭建步骤见：
[`../docs/Plan/ESP-IDF迁移与环境搭建方案.md`](../docs/Plan/ESP-IDF迁移与环境搭建方案.md)。

## 已验证的两个关键点

1. `sdkconfig.defaults` 必须包含 `CONFIG_FREERTOS_HZ=1000`，否则 CMake 配置报
   `esp32-arduino requires CONFIG_FREERTOS_HZ=1000 (currently 100)`。
2. arduino-esp32 作为组件时**不提供** `app_main`；`main/main.cpp` 必须自己定义
   `app_main` 并驱动 `setup()/loop()`，否则链接报 `undefined reference to app_main`。

## 在本机编译

~~~bash
export IDF_TOOLS_PATH=$HOME/.espressif
export PATH=/usr/bin:$PATH
. $HOME/esp/esp-idf/export.sh
cd MUS4_FW/idf_project
idf.py set-target esp32
idf.py build
~~~

## 接入现有固件（下一步）

1. 把 `../MUS4_FW.ino` 按 `main/main.cpp` 里的方式引入（`#include "MUS4_FW.ino"`，
   或改名 `MUS4_FW.cpp` 并放到 `main/`）。
2. 把 `../libraries/` 下的库包成 IDF 组件：优先用组件管理器
   （`idf_component.yml` 声明），其余为每个库在 `components/<name>/CMakeLists.txt` 注册
   `src` 源码与 include 目录。
3. 按业务需要改用本目录的 `partitions.csv`（对齐原 Arduino `min_spiffs`，4MB 双 app 分区）。

> `partitions.csv` 当前仅为参考模板，未在本工程启用；启用时在 `sdkconfig.defaults` 加
> `CONFIG_PARTITION_TABLE_CUSTOM=y` 与 `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"`。
