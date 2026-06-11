# 实施计划：Park 与 Drift 通道解耦并新增 CH5/CH6 输入

## Context

当前固件把 Park 与 Drift Assist 都绑定在 CH3 上：`park_change()` 既用 CH3 长按切换 Park，又用 CH3 三态阈值控制 Drift 开关并在中/高位自动解锁。这会导致 Park 点动按钮与 Drift 开关互相冲突，且 Drift 操作可能绕过用户期望的 Park 长按流程。

本次目标是把职责拆清：CH3 只作为按钮型点动通道控制 Park；新增 CH5 作为 Drift 开关，新增 CH6 作为 Drift 姿态修正强度旋钮。实现必须保留现有安全边界：Park 仍需要长按切换，Drift 不得自动解锁，`Serial1` 协议回传、命令 ACK/NACK、无线认证/OTA 与 Web Console 数据链路不应被改动。

## 修改文件

- `mus4.ino` — 扩展 RC 输入通道驱动，拆分 Park/Drift 控制，应用 CH6 修正比例。
- `TUI.h` / `TUI.cpp` — 同步显示 CH5/CH6，便于实机调试。
- `Doc/Hardware/pin_definitions.md` — 同步 CH5 GPIO27、CH6 GPIO35 的硬件定义。

## 现状定位

- `mus4.ino` 目前仅定义 `CH1_PIN`~`CH4_PIN` 与 4 路通道数组。
- PWM 采集相关的 `pwm_value`、`rise_time`、`last_valid_time`、滤波缓冲、`Channels`、`isr_functions` 和 `handle_interrupt()` 内部状态均按 4 通道组织。
- `park_change()` 已有 CH3 长按切换 Park 的状态机，且 `PARK_UNLOCK_HOLD_TIME = 1000`、`PARK_LOCK_HOLD_TIME = 500` 已符合需求。
- `park_change()` 末尾仍有 CH3 Drift 三态逻辑，会修改 `drift_assist_enabled` 并自动解锁 Park，是本次冲突根因。
- `apply_drift_assist()` 当前使用 `gyro_z_filtered * DRIFT_ASSIST_GAIN` 计算补偿并限幅到 `DRIFT_ASSIST_MAX_COMP`。
- TUI 与硬件文档目前只覆盖 CH1~CH4。

## 实施步骤

### 1. 扩展 RC 通道基础设施到 6 路

在 `mus4.ino` 中新增统一通道数量常量，避免继续散落硬编码 `4`：

- `RC_CHANNEL_COUNT = 6`
- `CH5_PIN 27`：接收机 PWM 输入 CH5，Drift 开关。
- `CH6_PIN 35`：接收机 PWM 输入 CH6，Drift 强度旋钮。
- `CH_DRIFT = 4`
- `CH_DRIFT_SCALE = 5`

把以下结构统一扩展为 `RC_CHANNEL_COUNT`：

- `pwm_value`
- `rise_time`
- `last_valid_time`
- `pwm_filter_buf`
- `pwm_filter_idx`
- `pwm_filtered`
- `Channels`
- `isr_functions`
- `handle_interrupt()` 内部状态数组
- `setup()` 中 attachInterrupt 循环
- `loop()` 中滤波循环
- `DEBUG` 通道打印循环

新增 `CH5_interrupt()` 与 `CH6_interrupt()`，分别调用 `handle_interrupt(CH_DRIFT)` 与 `handle_interrupt(CH_DRIFT_SCALE)`。

GPIO35 是输入专用且无内部上下拉，CH6 按普通 `INPUT` 初始化；GPIO27 保持与其他 RC 输入一致使用 `INPUT`，不额外启用下拉。

### 2. 让 CH3 只负责 Park 长按切换

保留 `park_change()` 的按钮长按状态机：

- CH3 `>1500` 视为按下。
- 当前 Park Locked 时，长按 `PARK_UNLOCK_HOLD_TIME = 1000ms` 解锁。
- 当前 Park Unlocked 时，长按 `PARK_LOCK_HOLD_TIME = 500ms` 上锁。
- 短按不改变 Park。

删除 `park_change()` 中 CH3 Drift 三态开关逻辑：

- 删除 `DRIFT_CH3_LOW_THRESHOLD` / `DRIFT_CH3_MID_THRESHOLD` 及其使用。
- 删除 CH3 中/高位自动解锁 Park 的行为。
- 删除 CH3 对 `drift_assist_enabled` 的写入。

同时把 `park_change()` 的输入改为已做超时默认处理后的 `pwm_filtered[CH_PARK]`，避免 RC 信号丢失时沿用最后一次 raw 高电平造成误判长按。

### 3. 新增独立 Drift 输入更新逻辑

新增独立函数 `update_drift_assist_control()`，不要再把 Drift 控制放在 `park_change()` 内。

逻辑约定：

- CH5 有效且 `pwm_filtered[CH_DRIFT] > 1500`：`drift_assist_enabled = true`。
- CH5 无效或 `<=1500`：`drift_assist_enabled = false`。
- Drift 关闭时立即清理运行态：`drift_assist_active = false`、`drift_compensation = 0.0f`、`gyro_z_filtered = 0.0f`。
- CH6 有效时按 `(constrain(CH6, 1000, 2000) - 1000) / 500.0f` 计算比例。
- CH6 无效时回到 `1.0f`，避免旋钮信号瞬断导致补偿强度突变。

新增全局状态：

- `float drift_assist_scale = 1.0f;`

推荐在主循环 RC 滤波和 Park/Mode 更新之后调用：

1. `park_change()`
2. `mode_change()`
3. `update_drift_assist_control()`
4. 后续控制融合与 `apply_drift_assist()`

### 4. 在 Drift 算法中应用 CH6 比例

在 `apply_drift_assist()` 中把 CH6 比例应用到陀螺仪补偿幅度：

- `raw_comp = gyro_z_filtered * DRIFT_ASSIST_GAIN * drift_assist_scale`

有效最大补偿也随比例缩放，但受最终转向范围保护：

- `effectiveMaxComp = min(DRIFT_ASSIST_MAX_COMP * drift_assist_scale, 100.0f)`
- `raw_comp = constrain(raw_comp, -effectiveMaxComp, effectiveMaxComp)`
- 最终 `final_steering` 继续保留 `constrain(final_steering, -100, 100)`。

这样 CH6=1500 保持旧版本约等于 70 的最大补偿；CH6=2000 可增强但不会超过最终转向物理范围；CH6=1000 时有效补偿为 0。

### 5. 同步 TUI/调试显示

同步扩展 TUI，便于实车调试新增通道：

- `TUI::setRC(int ch1, int ch2, int ch3, int ch4, int ch5, int ch6)`。
- `TUI::State` 增加 `ch5`、`ch6`。
- `drawRC()` dirty checking 覆盖 CH5/CH6。
- `drawRC()` 输出 CH1~CH6。
- `mus4.ino` 中 `tui.setRC(...)` 传入 `pwm_filtered[CH_DRIFT]` 与 `pwm_filtered[CH_DRIFT_SCALE]`。
- TUI 的 Drift 显示增加 `drift_assist_scale`。

### 6. 同步硬件文档

更新 `Doc/Hardware/pin_definitions.md`：

- 主表新增 GPIO27 / `CH5_PIN` / RC接收机 CH5 / Drift 开关。
- 主表新增 GPIO35 / `CH6_PIN` / RC接收机 CH6 / Drift 强度旋钮。
- RC 输入章节从 CH1~CH4 扩展为 CH1~CH6。
- 图示与迁移表补充 CH5/CH6。
- 注意事项中补充 GPIO35 为输入专用且无内部上下拉。

## 测试与验证

### 编译前检查

人工确认以下旧硬编码均已消除或有明确保留理由：

- RC/PWM 相关 `[4]`
- RC/PWM 相关 `for (int i = 0; i < 4; i++)`
- `DRIFT_CH3_LOW_THRESHOLD`
- `DRIFT_CH3_MID_THRESHOLD`
- `drift_assist_enabled` 由 CH3 写入
- CH3 中/高位自动解锁 Park

### 自动验证

运行现有 Python 回归，确保无线命令、Web 数据与工具逻辑未被破坏：

```powershell
pytest tests/
```

使用 WSL 加速编译固件：

```powershell
& "C:\Dev\FFE\Baoshan\mus4\arduino-cli-wsl.ps1" -c
```

### 实机验证

1. 断开动力或架空车轮后上电。
2. 确认 TUI/调试输出能看到 CH1~CH6 PWM。
3. CH3 Park：
   - 锁定状态下短按不解锁。
   - 锁定状态下长按 1s 解锁。
   - 解锁状态下短按不上锁。
   - 解锁状态下长按 0.5s 上锁。
4. CH5 Drift：
   - CH5<=1500 时 Drift OFF。
   - CH5>1500 时 Drift ARMED/ACTIVE。
   - CH5 切换不改变 Park。
5. CH6 Drift Scale：
   - CH6≈1000 时补偿接近 0。
   - CH6≈1500 时表现接近旧版本。
   - CH6≈2000 时补偿增强，但最终转向仍限制在 `-100~100`。
6. 模式边界：Drift 仍只在手动模式生效，半自动/全自动不介入。
7. 回归确认：`Serial1.printf("T%d:S%d\n", ...)` 回传、Web Console 状态显示、`/api/log`、OTA 命令与 ACK/NACK 响应正常。

## 风险与边界

- 不修改 `Serial1` 的 `Txx:Sxx` 协议回传。
- 不修改无线认证、Park 权限和 OTA 命令策略。
- 不改变控制输出最终 `-100~100` 限幅。
- `handle_interrupt()` 内部 static 数组必须同步扩到 6 路，这是最高风险越界点。
- GPIO35 只能输入且无内部上下拉，必须确认硬件 CH6 确实接到该脚。
- CH5 理论上是 1000/2000 切换通道，首版可用 `>1500` 判断；若实测在阈值附近抖动，再增加迟滞。
- CH6=2000 会比旧版补偿更激进，首次实车验证必须断开动力或架空车轮，并从低比例逐步增加。
