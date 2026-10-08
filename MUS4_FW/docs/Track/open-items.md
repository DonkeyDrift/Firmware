# 待办与验证边界追踪（Track）

> 本目录用于把 CHANGELOG / docs/Plan 里散落的「已设计未实现」「需在某环境回归」
> 类遗留项集中成可勾选清单，避免它们随文档滚动被淹没。每项都写清楚：
> **来源 → 缺什么 → 怎么才算完成**。
>
> 新增条目时请同步在 CHANGELOG 对应条目里加一句「已登记至 docs/Track/open-items.md」。

## T1 — WSL `-D` 编译回归（v1.10.15）

- **来源**：`CHANGELOG.md` v1.10.15「验证边界」——本次开发机为 Linux（无 PowerShell），
  `arduino-cli-wsl.ps1` 的 `-D/-Define` 与 `-BinTag` 仅通过括号/引号平衡检查
  与 pytest 静态断言验证。
- **缺什么**：Windows + WSL 环境下实际跑一次 `-D MUS4_BOARD_B` 编译，
  确认宏注入、`compiler.{c,cpp}.extra_flags` 读回、产物后缀推断三条路径都对。
- **完成判据**：
  1. `.arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino -D MUS4_BOARD_B -BinTag boardB` 在 Windows + WSL 成功；
  2. 产物 `MUS4_FW_boardB.bin` 生成且 `strings` 里含 `MUS4APP:CAR` 标记；
  3. 与 Linux 侧 `python arduino-cli.py -c -D MUS4_BOARD_B --bin-tag boardB` 的 bin 大小差异 < 1%（同一 commit）。
- **状态**：⬜ 未开始

## T2 — NVS 运行时切板（v1.10.14 第 5 步）

- **来源**：`CHANGELOG.md` v1.10.14 + `docs/Plan/主控板档案-NVS运行时切换方案.md`。
- **缺什么**：单镜像免重刷伺候两块主控板的运行时切换（目前只能 build 期 `-D` 切）。
- **完成判据**：
  1. NVS 存板型选择，boot 时读取并绑定 `serialTelemetry`/`serialConsole` 角色；
  2. Web Console / 串口命令可切换板型并重启生效；
  3. `BoardProfile.h` 的 `-D` 路径保持兼容（默认板 A 行为不变）。
- **状态**：⬜ 未开始（待出现「频繁互换且不愿重刷」的真实需求时启动）

## T3 — Web Console「进入 donkey / 进入 DonkeyDrifter」按钮跳转

- **来源**：`CHANGELOG.md` 1641 行——按钮已加但跳转**功能预留，暂未实现**。
- **缺什么**：按钮点击后跳转到 DonkeyDrift launcher / DonkeyDrifter 对应页面。
- **完成判据**：按钮点击后在新标签打开目标 URL；URL 可通过 Web Console 配置；
  断网/未配置时按钮置灰并有提示。
- **状态**：⬜ 未开始

## T4 — Playwright provisioning 测试占位脚本

- **来源**：`README.md` Tests 段——`provisioning_system/playwright_tests/` 的
  `npm test` 当前是占位，直接 `exit 1`。
- **缺什么**：真实 e2e 用例（配网流程、Web UI 弹窗、AP→STA 切换）。
- **完成判据**：`npm test` 在 `provisioning_system/playwright_tests/` 下跑通至少
  3 条核心路径（配网成功 / 密码错 / AP 未发现），CI 可选加入。
- **状态**：⬜ 未开始

## T5 — IMU 环形缓冲 + 上位机批量拉取

- **来源**：`docs/Inspect/drive-loop-hz-and-ring-buffer-analysis.md`——IMU 500Hz→100Hz→
  Vehicle 20-60Hz 三层降采样，200ms 漂移只剩 4-12 样本；文档提出 ESP32 侧环形缓冲方案但未实现。
- **缺什么**：固件侧 IMU 环形缓冲 + 新下行命令拉取批量样本 + DonkeyDrift 侧 `ArdImu` 批量消费。
- **完成判据**：
  1. 固件提供 `IMU_PULL` 类命令，一次返回 N 个连续样本（含 seq/ts）；
  2. DonkeyDrift 侧 `Arduino` 类批量解析并写入 Tub（不丢中间样本）；
  3. 对比测试：200ms 漂移窗口内捕获样本数 ≥ 50（vs 现状 4-12）。
- **状态**：⬜ 未开始（**跨仓库**，需与 DonkeyDrift 同步推进；schema 已在
  `protocol/mus4_serial_v1.yaml` 的 known_issues.imu-loss 记录）

## T6 — `DRIVE_LOOP_HZ=60` 推理基准验证

- **来源**：`docs/Inspect/drive-loop-hz-and-ring-buffer-analysis.md`——若模型推理 P95 > 16.7ms，
  60Hz 是空转；当前无自动化基准。
- **缺什么**：把 `PartProfiler` 输出纳入 CI 回归，锁定模型推理 P50/P95。
- **完成判据**：CI 输出各 Part 耗时分布；推理 P95 超阈值时红灯（或至少产出报告 artifact）。
- **状态**：⬜ 未开始（**跨仓库**，落点在 DonkeyDrift CI）
