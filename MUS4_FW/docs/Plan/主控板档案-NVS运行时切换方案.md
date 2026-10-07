# 主控板档案 · NVS 运行时切换方案（第 5 步：已设计，未实现）

> **状态：待办 / 未实现**（2026-10-07 记录，经评估暂缓）。
> 现行实现是**编译期档案**：`MUS4_FW/libraries/mus4_core/src/BoardProfile.h`（默认板 A，`-D MUS4_BOARD_B` 切板，产物 `tools/build_profiles.sh`）。
> 本文记录已推演清楚的运行时方案，供日后启动时直接开工。

## 1. 动机与取舍

| | 编译期档案（已实现） | NVS 运行时切换（本方案） |
| --- | --- | --- |
| 换板动作 | 重新编译 + 烧录/OTA（约 10–60s） | Web Console / 命令行一点即换，重启生效，**免重刷** |
| 镜像数量 | 每块板一个 bin | **一个 bin 伺候两块板** |
| 实现成本 | 低（已完成） | 中：角色绑定要从静态引用改成可变绑定 |
| 风险 | 低 | 启动顺序耦合、状态错配（接 A 板却跑 B 配置） |

**启动条件**：出现"同一块 ESP32 需要频繁在两块主控板之间互换、且不愿重刷"的真实需求时再做；在此之前编译期档案已覆盖需求。

## 2. 设计要点

1. **存储**：NVS namespace `board`，key `swap`（u8 0/1）。读取放在 `setup()` 最早处、**任何 `Serial*.begin()` 之前**；NVS 缺失/损坏 → 回落 `BoardProfile.h` 的档案默认值。
2. **角色绑定改造**（核心成本）：
   - `SerialRole.h` 现为 `extern HardwareSerial& serialTelemetry / serialConsole` 静态绑定 → 改为访问函数 `HardwareSerial& serialTelemetryPort()`（内部按运行时标志返回 `Serial` 或 `Serial1`），或全局指针 + `setup()` 早期赋值。
   - `MUS4_FW.ino` 的 `TUI tui(serialConsole);` 在**全局构造期**绑定 `Print&` 引用，引用不可重绑 → TUI 需改为指针成员 + `setOutput(Print&)`，或延迟到 `setup()` 内构造。
   - `Mus4Log.cpp`、`SerialLineReader.cpp` 已按角色调用，只需改绑定方式；`MUS4_FW.ino` 中 `Serial.begin()/Serial1.begin()` 等硬件初始化**保持不变**（引脚、波特率与档案无关）。
3. **切换入口**：
   - Web Console 设置页下拉"主控板型号"（复用既有设置视图）；
   - 命令行 `BOARD SWAP ON|OFF|?`（走 `mus4_command` 分发器）；
   - `GET /api/status` 增加 `board_profile` 字段，`GET /api/board-info`（可选）返回当前值与可选值。
4. **可观测与防错配**：开机 banner 打印当前档案；Web Console 页头显示；切换写 NVS 后返回 `ACK:REBOOT_REQUIRED`，重启生效（避免运行中改绑导致半帧数据丢失）。
5. **与 OTA 槽位维度正交**：槽位 = 镜像用途（`MUS4APP:CAR` / `MUS4APP:BRIDGE`），档案 = 通信配置；状态字段、bin 命名、CHANGELOG 措辞都分开表述，不混用。

## 3. 影响面清单

| 文件 | 改动 |
| --- | --- |
| `libraries/mus4_core/src/SerialRole.h` | 静态引用 → 访问函数 / 指针 + 运行时标志 |
| `libraries/mus4_core/src/BoardProfile.h` | 增加"出厂默认值"查询接口；保留编译期档案作为回落 |
| `MUS4_FW.ino` | `setup()` 早期读 NVS；TUI 延迟绑定；banner 打印档案 |
| `libraries/mus4_ui/src/TUI.h/.cpp` | `Print&` 成员 → 可重绑（指针 + `setOutput`） |
| `libraries/mus4_web/src/WebConsoleServer.cpp` | `/api/status` 增加 `board_profile`；新增切换端点（鉴权 + Park Locked 同款流程） |
| `libraries/mus4_web/src/WebConsoleAssets.h` | 设置页"主控板型号"控件 |
| `libraries/mus4_command/*` | `BOARD` 命令 |
| `tests/test_firmware_feature_flags.py` | 断言从"静态绑定"改为"运行时路由 + 回落默认" |

## 4. 验收清单

- [ ] 板 A / 板 B 各自编译通过，且同一 bin 可在两块板上运行
- [ ] 切换 + 重启后遥测口确实改变，`/api/status` 的 `board_profile` 正确
- [ ] NVS 清空 / 读取失败时回落档案默认值
- [ ] 错配防护：接 A 板跑 B 配置时，banner 与 Console 有明确提示
- [ ] 测试全绿；`tools/build_profiles.sh` 矩阵仍通过
- [ ] `README.md` / `README.zh-CN.md` / `docs/Guide/esp32-serial-topology.md` 补运行时说明

## 5. 相关

- 编译期档案：`libraries/mus4_core/src/BoardProfile.h`
- 运行时配置先例：`libraries/mus4_core/src/MutePreference.cpp`（NVS + API + 设置页）、`mus4_wifi/WifiStaConfig.cpp`
- 角色路由现状：`libraries/mus4_core/src/SerialRole.h`、`docs/Inspect/serial-output-logic-and-feature-config.md`
