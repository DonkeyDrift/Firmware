# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 仓库定位

`C:\Dev\DDC\Firmware` 是 **DonkeyDrift 项目的固件仓库根**，本身不放任何固件代码，只承担"多固件子项目容器"的角色。具体的源码、构建脚本、文档与测试全部下沉到子项目目录中，每个子项目自包含、独立构建。

当前在驻子项目：

- [`MUS4_FW/`](MUS4_FW/) — MUS4（LP-MU-S4）基于 ESP32 + Arduino framework 的遥控车辆/机器人底层控制固件。当前固件版本以 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) 顶部声明为准，避免在根级文件里维护版本号。

新增其它硬件平台的固件时，应作为根目录下并列的同级子目录引入，目录命名采用 `<硬件型号>_FW/` 形式（全大写硬件代号 + `_FW` 后缀），与现存 `MUS4_FW/` 保持一致；各自维护自己的 `README.md`、`CLAUDE.md`、`.gitignore`、构建脚本与测试，不要把代码上提到仓库根。

MUS4 子项目内还包含两个不遵循 `<硬件型号>_FW/` 命名规范的辅助性/实验性子系统，各自独立维护：
- `MUS4_FW/multi_agent_framework/` — 独立 Python 多智能体协作框架（IPC 消息队列、ESP-IDF 示例固件、Linux 脚本、WebSocket 控制面板），不属于 ESP32 固件主链路。
- `MUS4_FW/provisioning_system/` — 独立 Wi-Fi 配网系统（ESP32 AP Web Server + Linux agent + Playwright 测试资源），通过 UART 把 Wi-Fi 凭据从 ESP32 传递到 Linux 主机上的 `nmcli`。

## 工作流：必须先进入子项目

仓库根没有 sketch、没有构建脚本、也没有 Python 测试。**任何"构建/烧录/测试/运行 Python 工具"的操作都必须先 `cd` 到对应子项目根**，然后遵循该子项目自己的 `CLAUDE.md`。

常用操作速查（均需先 `cd MUS4_FW`）：

```powershell
# 仅编译验证（修改固件后首选）
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino

# 编译 + HTTP OTA 上传到调试设备
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost 192.168.3.52 -Sketch MUS4_FW.ino

# 运行全部 Python 测试
pytest tests/

# 运行单个测试文件/用例
pytest tests/test_firmware_feature_flags.py
pytest tests/test_wireless_console_policy.py -k "test_requires_authentication"
```

- MUS4 固件相关任务 → `cd MUS4_FW` 后阅读并遵循 [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md)。该文件覆盖了：
  - WSL 加速构建脚本 `arduino-cli-wsl.ps1` 与 Windows 端 `arduino-cli.py` 的常用命令矩阵；
  - HTTP OTA 默认目标读取 `.mus4_ota_target`、`config.yaml`/`sketch.yaml`/`wslbuild.yaml` 的优先级；
  - `MUS4_FW.ino` 主 sketch 与 `libraries/mus4_{core,ui,rc,control,safety}/src/` 模块边界（v1.7.4 模块化拆分结果）；
  - Park/紧急制动、PWM 300Hz/14bit、`IRAM_ATTR` 中断等**安全关键编辑约束**；
  - 无线 Console（AP+STA、TCP 2323、Web 80、WebSocket 81、ArduinoOTA 3232）的权限分层；
  - `wireless_console_policy.py` 镜像策略、`tests/test_firmware_feature_flags.py` 源码断言等需要联动更新的位置；
  - 当前固件版本号也在该文件顶部维护，不要从根级 `CLAUDE.md` 推断。

跨子项目的通用规则（语言、TDD、Conventional Commits）由用户全局 `~/.claude/CLAUDE.md` 提供，本文件不再重复。

## 自动化行为

MUS4 子项目的 `.claude/settings.local.json` 配置了 `PostToolUse` 钩子：当通过 PowerShell 执行 `arduino-cli-wsl.ps1` 编译成功后，自动追加 HTTP OTA 上传到预配置目标设备。设置环境变量 `$env:MUS4_HOOK_DRY_RUN=1` 可仅打印 would-do 消息而不实际执行上传。

## 仓库根级约定

- **`.gitignore` 分层**：根 `.gitignore` 只覆盖 OS / 编辑器层面的全局垃圾（IDE 元数据、`.DS_Store` 之类）。构建产物、密钥模板、本地工具缓存等子项目相关忽略项**必须**写入子项目自己的 `.gitignore`，不要上提。两个 `.gitignore` 是互补关系（非继承关系）：判断未跟踪文件归属时，若路径在子项目下先查子项目 `.gitignore`，仅出现在根目录时才查根 `.gitignore`。在根目录看到未跟踪文件时，先确认它属于哪个子项目，再决定移动到该子项目内或扩充对应 `.gitignore`，不要在根 `.gitignore` 里继续堆叠子项目级条目。
- **不要在根目录添加源代码或可执行脚本**。若工具脚本只服务于某一子项目，归入 `<子项目>/tools/` 或 `<子项目>/` 根。
- **历史提交可见**：当前 git 主分支为 `main`，最近一次根级重构把固件从仓库根下沉到了 `MUS4_FW/`（具体提交见 `git log -- MUS4_FW/`）；引用旧路径的文档或脚本若仍存在，应在所属子项目内修正而不是在根目录新增 shim。

## 文档入口

- [`AGENTS.md`](AGENTS.md) — 仓库根级面向通用 AI 编码代理的指南，与本文档互补。
- [`README.md`](README.md) — 仓库根 README，只说明当前在驻子项目清单。
- [`MUS4_FW/README.md`](MUS4_FW/README.md) / [`MUS4_FW/README.zh-CN.md`](MUS4_FW/README.zh-CN.md) — MUS4 子项目对外介绍。
- [`MUS4_FW/CLAUDE.md`](MUS4_FW/CLAUDE.md) — MUS4 子项目的权威工作指南；进入该子项目工作前优先读它。
- [`MUS4_FW/docs/`](MUS4_FW/docs/) — MUS4 架构、硬件引脚、工具、计划文档目录（部分历史描述可能滞后，引用前对照源码）。
- [`docs/superpowers/`](docs/superpowers/) — 根级唯一的文档目录，存放 superpowers 技能产出的跨切面 `plans/`（实现计划，含 `- [ ]` 任务清单）与 `specs/`（设计稿）。当前内容均针对 MUS4 固件（如 Wi-Fi 蜂鸣器提示音），引用前同样对照 `MUS4_FW/` 源码验证。
