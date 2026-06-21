# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 仓库定位

`C:\Dev\DDC\Firmware` 是 **DonkeyDrift 项目的固件仓库根**，本身不放任何固件代码，只承担"多固件子项目容器"的角色。具体的源码、构建脚本、文档与测试全部下沉到子项目目录中，每个子项目自包含、独立构建。

当前在驻子项目（截至 2026/06/21）：

- [`MUS4_FW/`](MUS4_FW/) — MUS4（LP-MU-S4）基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。

新增其它硬件平台的固件时，应作为根目录下并列的同级子目录引入（例如 `XYZ_FW/`），并各自维护自己的 `README.md`、`CLAUDE.md`、`.gitignore`、构建脚本与测试，不要把代码上提到仓库根。

## 工作流：必须先进入子项目

仓库根没有 sketch、没有构建脚本、也没有 Python 测试。**任何"构建/烧录/测试/运行 Python 工具"的操作都必须先 `cd` 到对应子项目根**，然后遵循该子项目自己的 `CLAUDE.md`：

- MUS4 固件相关任务 → `cd MUS4_FW` 后阅读并遵循 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md)。该文件覆盖了：
  - WSL 加速构建脚本 `arduino-cli-wsl.ps1` 与 Windows 端 `arduino-cli.py` 的常用命令矩阵；
  - HTTP OTA 默认目标读取 `.mus4_ota_target`、`config.yaml`/`sketch.yaml`/`wslbuild.yaml` 的优先级；
  - `MUS4_FW.ino` 主 sketch 与 `libraries/mus4_{core,ui,rc,control,safety}/src/` 模块边界（v1.7.4 模块化拆分结果）；
  - Park/紧急制动、PWM 300Hz/14bit、`IRAM_ATTR` 中断等**安全关键编辑约束**；
  - 无线 Console（AP+STA、TCP 2323、Web 80、WebSocket 81、ArduinoOTA 3232）的权限分层；
  - `wireless_console_policy.py` 镜像策略、`tests/test_firmware_feature_flags.py` 源码断言等需要联动更新的位置。

跨子项目的通用规则（语言、TDD、Conventional Commits）由用户全局 `~/.claude/CLAUDE.md` 提供，本文件不再重复。

## 仓库根级约定

- **`.gitignore` 分层**：根 `.gitignore` 只覆盖 OS / 编辑器层面的全局垃圾（IDE 元数据、`.DS_Store` 之类）。构建产物、密钥模板、本地工具缓存等子项目相关忽略项**必须**写入子项目自己的 `.gitignore`，不要上提。
- **不要在根目录添加源代码或可执行脚本**。若工具脚本只服务于某一子项目，归入 `<子项目>/tools/` 或 `<子项目>/` 根。
- **历史提交可见**当前 git 主分支为 `main`，最近一次重构 `e448228` 把固件从仓库根下沉到了 `MUS4_FW/`；引用旧路径的文档或脚本若仍存在，应在所属子项目内修正而不是在根目录新增 shim。

## 文档入口

- [`README.md`](README.md) — 仓库根 README，只说明当前在驻子项目清单。
- [`MUS4_FW/README.md`](MUS4_FW/README.md) / [`MUS4_FW/README.zh-CN.md`](MUS4_FW/README.zh-CN.md) — MUS4 子项目对外介绍。
- [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) — MUS4 子项目的权威工作指南；进入该子项目工作前优先读它。
- [`MUS4_FW/Doc/`](MUS4_FW/Doc/) — MUS4 架构、硬件引脚、工具、计划文档目录（部分历史描述可能滞后，引用前对照源码）。
