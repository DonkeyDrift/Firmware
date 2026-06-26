# 手柄/摇杆零位与正负最大值校准设计

> 方案：C（统一 `JoystickCalibration` 模块）
> 日期：2026-06-26
> 子项目：`MUS4_FW`

## 1. 目标

由于小车底盘结构限制，RC 手柄/摇杆的物理中位输出并不在电气零位，且正、反方向的最大行程也存在个体差异。本设计新增一套统一的摇杆校准功能，覆盖**方向（Steering）**与**油门（Throttle）**两轴：

- 在 Drift Console 中提供带文字引导的两步校准向导；
- 校准值（每轴 `min/mid/max`）持久化到 ESP32 NVS；
- 在 `ControlMixer` 归一化阶段消除偏差，使手动 RC 数据与模型/主机数据对齐；
- 适用于 MANUAL、SEMI_AUTO、FULL_AUTO 三种模式（其中 FULL_AUTO 通过统一的归一化基准间接受益）。

## 2. 背景与现状

当前代码库已有针对方向的 `SteeringCalibration`：

- 文件：`MUS4_FW/libraries/mus4_control/src/SteeringCalibration.cpp/.h`
- 功能：保存 `{min_pwm, mid_pwm, max_pwm}`，提供 `mapSteeringCalibrated()`
- 使用点：`ControlMixer.cpp` 的 MANUAL 分支
- 命令：`STEER_CAL`、`CAL_SAVE`、`CAL_RETRY`、`CAL_ABORT`、`CAL_RESET`、`CAL_STATUS`
- 权限：`PARK_LOCKED_COMMANDS`，需认证

油门目前没有校准，仍使用编译期宏 `RC_THROTTLE_MIN/MID/MAX`：

```cpp
car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
```

所有模式最终都在 `ControlMixer::updateControlOutput()`  converges 到 `[-100, 100]`，再经漂移辅助、Park/安全限幅、PWM 输出。模型/主机指令已是 `[-100, 100]`，因此只需在 RC→归一化阶段做校准即可对齐。

## 3. 设计原则

1. **单一职责**：新的 `JoystickCalibration` 模块负责采集、存储、映射两轴校准数据。
2. **向后兼容**：启动时尝试读取旧的 NVS 方向盘校准 key；若存在则迁移到新 key，旧 key 保留作为备份。
3. **安全关键**：校准命令仍走 Park-locked + 认证路径；采集期间不写入 actuator。
4. **UI 友好**：Drift Console 用模态向导 + 实时数值显示，文字引导用户完成两步操作。
5. **可测试**：映射函数独立、纯函数，便于单元测试；状态机边界清晰。

## 4. 架构概览

```text
RC PWM → RcFilter → pwm_filtered[]
                          ↓
              JoystickCalibration
              (min/mid/max per axis)
                          ↓
              ControlMixer::updateControlOutput()
              mapJoystickAxis() → [-100, 100]
                          ↓
              DriftAssist / Safety / ActuatorOutput
```

新增/修改文件：

| 文件 | 动作 | 说明 |
|---|---|---|
| `libraries/mus4_control/src/JoystickCalibration.h` | 新增 | 公共 API、状态机枚举、结构体 |
| `libraries/mus4_control/src/JoystickCalibration.cpp` | 新增 | 采集、映射、NVS 读写、迁移 |
| `libraries/mus4_control/src/SteeringCalibration.h/.cpp` | 删除 | 功能被合并，避免重复 |
| `libraries/mus4_control/src/ControlMixer.cpp` | 修改 | 使用新映射函数 |
| `libraries/mus4_core/src/WifiConsoleTypes.h` | 修改 | 新增 NVS key 常量 |
| `libraries/mus4_core/src/RuntimeState.h` | 可能修改 | 若需要把校准状态暴露给 Web |
| `MUS4_FW.ino` | 修改 | 启动加载调用替换 |
| `libraries/mus4_command/src/CommandDispatcher.cpp` | 修改 | 替换/新增校准命令 |
| `libraries/mus4_command/src/WirelessConsole.cpp` | 修改 | 权限分类同步 |
| `libraries/mus4_web/src/WebConsoleAssets.h` | 修改 | 新增校准向导 UI |
| `libraries/mus4_web/src/WebConsoleServer.cpp` | 可能修改 | 可选 `/api/joystick_cal` 端点 |
| `wireless_console_policy.py` | 修改 | 新命令权限同步 |
| `tests/test_wireless_console_policy.py` | 修改 | 新命令测试 |
| `tests/test_joystick_calibration.py` | 新增 | 映射函数与状态机测试 |

## 5. 数据模型

### 5.1 校准记录

```cpp
#pragma once
#include <cstdint>

struct AxisCalibration {
    int16_t min_pwm = 0;
    int16_t mid_pwm = 0;
    int16_t max_pwm = 0;
};

struct JoystickCalibrationData {
    AxisCalibration steering;
    AxisCalibration throttle;
    bool steering_enabled = false;
    bool throttle_enabled = false;
};
```

默认值从 `FirmwareConfig.h` 读取：

```cpp
#define JOYSTICK_STEERING_MIN_DEFAULT  RC_STEERING_MIN
#define JOYSTICK_STEERING_MID_DEFAULT  RC_STEERING_MID
#define JOYSTICK_STEERING_MAX_DEFAULT  RC_STEERING_MAX
#define JOYSTICK_THROTTLE_MIN_DEFAULT  RC_THROTTLE_MIN
#define JOYSTICK_THROTTLE_MID_DEFAULT  RC_THROTTLE_MID
#define JOYSTICK_THROTTLE_MAX_DEFAULT  RC_THROTTLE_MAX
```

### 5.2 NVS Key

在 `WifiConsoleTypes.h` 新增：

```cpp
#define MUS4_PREF_JOYSTICK_NAMESPACE    "mus4"

#define MUS4_PREF_JOYSTICK_STEER_MIN_KEY  "js_st_min"
#define MUS4_PREF_JOYSTICK_STEER_MID_KEY  "js_st_mid"
#define MUS4_PREF_JOYSTICK_STEER_MAX_KEY  "js_st_max"
#define MUS4_PREF_JOYSTICK_STEER_EN_KEY   "js_st_en"

#define MUS4_PREF_JOYSTICK_THROT_MIN_KEY  "js_th_min"
#define MUS4_PREF_JOYSTICK_THROT_MID_KEY  "js_th_mid"
#define MUS4_PREF_JOYSTICK_THROT_MAX_KEY  "js_th_max"
#define MUS4_PREF_JOYSTICK_THROT_EN_KEY   "js_th_en"
```

旧 key（用于迁移）：

```cpp
#define MUS4_PREF_STEER_MIN_KEY  "str_min"
#define MUS4_PREF_STEER_MID_KEY  "str_mid"
#define MUS4_PREF_STEER_MAX_KEY  "str_max"
#define MUS4_PREF_STEER_CAL_EN_KEY "str_cal"
```

## 6. 校准状态机

```cpp
enum class JoystickCalState {
    IDLE,       // 未在采集
    CENTERING,  // 采集中位
    MINMAX,     // 采集正负最大值
    DONE        // 采集完成，等待保存/重试
};
```

### 6.1 状态转换

```text
IDLE --[JOYSTICK_CAL]--> CENTERING --(timeout/average stable)--> MINMAX --(timeout)--> DONE
 ^                          |                                       |
 |                          v                                       v
 +---[JOYSTICK_ABORT/RESET]----+                          [JOYSTICK_SAVE]
```

### 6.2 采集算法

**中位采集（CENTERING）**：

- 进入状态时清空两轴滑动窗口（长度 20，约 400 ms @ 50 Hz）。
- 每循环读取 `pwm_filtered[CH_STEERING]`、`pwm_filtered[CH_THROTTLE]`。
- 当窗口满且连续 10 个样本与当前中值偏差均 ≤ 6 µs 时，认为中位稳定，取滑动中值作为 `mid_pwm`。
- 超时 3 秒未稳定也退出，但 UI 提示“采集可能不稳定”。

**最大值采集（MINMAX）**：

- 进入状态时初始化 `min_pwm = INT16_MAX`、`max_pwm = INT16_MIN`。
- 持续更新两轴最小/最大值。
- 固定 5 秒后自动进入 `DONE`。
- UI 实时显示当前已记录的极值，提示用户继续推动摇杆以覆盖更大范围。

### 6.3 验证

保存前必须满足：

```cpp
min < mid < max
(mid - min) > 100
(max - mid) > 100
min >= RC_PWM_MIN  // 800
max <= RC_PWM_MAX  // 2200
```

任一轴不满足则返回 `NACK:INVALID_CALIBRATION`。

## 7. 映射函数

所有映射在 `JoystickCalibration.cpp` 中实现为纯函数：

```cpp
int mapJoystickAxis(int16_t pwm,
                    const AxisCalibration& cal,
                    bool enabled,
                    int16_t default_min,
                    int16_t default_mid,
                    int16_t default_max) {
    int16_t min_pwm = enabled ? cal.min_pwm : default_min;
    int16_t mid_pwm = enabled ? cal.mid_pwm : default_mid;
    int16_t max_pwm = enabled ? cal.max_pwm : default_max;

    if (pwm < mid_pwm) {
        int v = map(pwm, min_pwm, mid_pwm, -100, 0);
        return constrain(v, -100, 0);
    }
    int v = map(pwm, mid_pwm, max_pwm, 0, 100);
    return constrain(v, 0, 100);
}
```

为保持与旧 `mapSteeringCalibrated` 行为一致，注意 `map()` 在 Arduino 中做整数截断；输出按侧钳位到 `[-100, 0]` 或 `[0, 100]`，最终由 `ControlMixer` 与 `ActuatorOutput` 再统一限幅。

调用示例：

```cpp
car_output.throttle = mapJoystickAxis(rc_data.throttle,
                                      joystick_cal.throttle,
                                      joystick_cal.throttle_enabled,
                                      RC_THROTTLE_MIN,
                                      RC_THROTTLE_MID,
                                      RC_THROTTLE_MAX);
```

## 8. ControlMixer 集成

在 `ControlMixer::updateControlOutput()` 中替换原有映射：

```cpp
// 替换前：
// car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
// car_output.steering = steer_cal_enabled ? mapSteeringCalibrated(rc_data.steering)
//                                         : map(...);

// 替换后：
car_output.throttle = mapJoystickAxis(rc_data.throttle,
                                      joystick_cal.throttle,
                                      joystick_cal.throttle_enabled,
                                      RC_THROTTLE_MIN,
                                      RC_THROTTLE_MID,
                                      RC_THROTTLE_MAX);

car_output.steering = mapJoystickAxis(rc_data.steering,
                                      joystick_cal.steering,
                                      joystick_cal.steering_enabled,
                                      RC_STEERING_MIN,
                                      RC_STEERING_MID,
                                      RC_STEERING_MAX);
```

模式影响：

| 模式 | 油门来源 | 方向来源 | 校准应用 |
|---|---|---|---|
| MANUAL | RC 油门 → `mapJoystickAxis(throttle)` | RC 方向 → `mapJoystickAxis(steering)` | 两轴都校准 |
| SEMI_AUTO | RC 油门 → `mapJoystickAxis(throttle)` | 模型/主机方向 + DriftAssist | 油门校准 |
| FULL_AUTO | 模型/主机油门 | 模型/主机方向 + DriftAssist | 模型数据已是 `[-100,100]`，使用同一归一化基准，无需额外处理 |

## 9. Drift Console UI

### 9.1 入口

在 RC Channels 折叠面板旁新增按钮：

```html
<button onclick="openJoystickCalModal()" data-i18n="btn.joystickCal">手柄校准</button>
```

### 9.2 状态显示（常驻）

在 RC Channels 面板下方或独立折叠面板显示当前校准值：

```text
方向校准: min=xxx  mid=xxx  max=xxx  [启用/禁用]
油门校准: min=xxx  mid=xxx  max=xxx  [启用/禁用]
```

### 9.3 校准向导模态框

模态框分三步：

1. **中位采集**
   - 文字："第 1 步：请将手柄（方向和油门）完全回中，保持不动，然后点击【开始采集】。"
   - 显示实时 PWM：方向、油门
   - 按钮：【开始采集】/【取消】
   - 采集完成后自动进入下一步

2. **正负最大值采集**
   - 文字："第 2 步：请在倒计时内将手柄依次推到最大位置：方向左、方向右、油门前、油门后。"
   - 显示当前已记录的 min/max
   - 倒计时进度条（5 秒）
   - 按钮：【完成】/【重采】/【取消】

3. **确认与保存**
   - 显示四组值
   - 开关：启用方向校准 / 启用油门校准
   - 按钮：【保存到校车】/【重试】/【恢复默认】/【取消】

### 9.4 后端命令

优先复用 `/api/cmd?target=web`，新增命令：

| 命令 | 动作 |
|---|---|
| `JOYSTICK_CAL` | 进入 CENTERING 状态 |
| `JOYSTICK_MINMAX` | 从 CENTERING 进入 MINMAX（或从 IDLE 直接进入 MINMAX） |
| `JOYSTICK_SAVE` | 保存当前采集值到 NVS |
| `JOYSTICK_RETRY` | 回到 CENTERING 重新采集 |
| `JOYSTICK_ABORT` | 放弃采集，回到 IDLE |
| `JOYSTICK_RESET` | 恢复默认值并清除 NVS |
| `JOYSTICK_STATUS` | 返回当前校准值和状态 |

若后续 UI 需要 JSON 更友好，可新增 `/api/joystick_cal`：

- `GET`：返回 `{steering:{min,mid,max,enabled}, throttle:{...}, state}`
- `POST`：接收 `{action:"start"|"minmax"|"save"|"retry"|"abort"|"reset", steeringEnabled, throttleEnabled}`

## 10. 命令权限与安全

新增命令全部加入 `PARK_LOCKED_COMMANDS`：

```python
PARK_LOCKED_COMMANDS = {
    "TEST", "TEST_TUI", "BENCH", "STRESS", "REGRESS", "FILTER_TEST",
    # 旧命令保留兼容一个版本
    "STEER_CAL", "CAL_SAVE", "CAL_RETRY", "CAL_ABORT", "CAL_RESET", "CAL_STATUS",
    # 新命令
    "JOYSTICK_CAL", "JOYSTICK_MINMAX", "JOYSTICK_SAVE", "JOYSTICK_RETRY",
    "JOYSTICK_ABORT", "JOYSTICK_RESET", "JOYSTICK_STATUS",
}
```

固件 `WirelessConsole.cpp` 同步更新分类。

Web Console 可在 `park == false` 时禁用向导开始按钮，但最终权限校验由固件负责，返回 `NACK:PARK_REQUIRED`。

## 11. 旧数据迁移

`loadJoystickCalibration()` 启动逻辑：

```cpp
void loadJoystickCalibration() {
    prefs().begin(MUS4_PREF_NAMESPACE, true);

    // 1. 方向盘：优先读新 key；若新 key 不存在且存在旧 key，则迁移
    if (prefs().isKey(MUS4_PREF_JOYSTICK_STEER_MIN_KEY)) {
        joystick_cal.steering.min_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MIN_KEY, RC_STEERING_MIN);
        joystick_cal.steering.mid_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MID_KEY, RC_STEERING_MID);
        joystick_cal.steering.max_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MAX_KEY, RC_STEERING_MAX);
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_STEER_EN_KEY, false);
    } else if (prefs().isKey(MUS4_PREF_STEER_MIN_KEY)) {
        joystick_cal.steering.min_pwm = prefs().getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        joystick_cal.steering.mid_pwm = prefs().getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        joystick_cal.steering.max_pwm = prefs().getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
        // 立即写入新 key，下次启动不再走迁移分支
        prefs().end();
        saveJoystickCalibration();
        return;
    } else {
        joystick_cal.steering.min_pwm = RC_STEERING_MIN;
        joystick_cal.steering.mid_pwm = RC_STEERING_MID;
        joystick_cal.steering.max_pwm = RC_STEERING_MAX;
        joystick_cal.steering_enabled = false;
    }

    // 2. 油门：只有新 key
    joystick_cal.throttle.min_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MIN_KEY, RC_THROTTLE_MIN);
    joystick_cal.throttle.mid_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MID_KEY, RC_THROTTLE_MID);
    joystick_cal.throttle.max_pwm = prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MAX_KEY, RC_THROTTLE_MAX);
    joystick_cal.throttle_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_THROT_EN_KEY, false);

    prefs().end();
}
```

迁移完成后，旧 key 可保留一个版本作为备份，再后续版本中移除读取逻辑。

## 12. 测试计划

### 12.1 单元测试

新增 `MUS4_FW/tests/test_joystick_calibration.py`：

- `mapJoystickAxis` 边界：
  - `pwm == mid` → `0`
  - `pwm == min` → `-100`
  - `pwm == max` → `100`
  - `pwm < min` → 钳位 `-100`
  - `pwm > max` → 钳位 `100`
  - disabled 时使用默认值线性映射
- 状态机转换：
  - `IDLE → CENTERING → MINMAX → DONE`
  - `ABORT` 任意状态回到 `IDLE`
  - `RESET` 清除数据
- 验证规则：
  - `min >= mid` 应拒绝保存
  - `mid - min <= 100` 应拒绝保存

### 12.2 策略测试

更新 `tests/test_wireless_console_policy.py`：

- 新命令全部在 `PARK_LOCKED_COMMANDS` 中
- 新命令需要认证

### 12.3 集成测试

- 编译验证：`cd MUS4_FW && .\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`
- 实车验证：
  - 中位校准后，MANUAL 模式下手柄回中，舵机/电调输出接近零位；
  - SEMI_AUTO 模式下油门响应与 FULL_AUTO 模型油门在量纲上对齐；
  - 模式切换无明显跳变。

## 13. 风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| 删除 `SteeringCalibration` 导致外部引用断裂 | 编译失败 | 全局搜索替换；在 `JoystickCalibration.h` 中提供 `mapSteeringCalibrated()` 内联别名，转发到 `mapJoystickAxis()`，保留一个版本后再移除 |
| 旧 NVS 数据迁移失败 | 用户丢失已有方向盘校准 | 启动时先读旧 key；保存时同时写入新 key；保留旧 key 至少一个版本 |
| 采集期间摇杆抖动导致中位不准 | 零位偏差未消除 | 滑动窗口 + 稳定阈值；允许用户重试 |
| 新命令权限分类遗漏 | 安全漏洞 | 同时更新 `wireless_console_policy.py`、固件 `WirelessConsole.cpp`、并跑策略测试 |
| UI 向导与命令状态不同步 | 用户体验差 | 提供 `JOYSTICK_STATUS` 轮询；采集超时后自动进入 DONE |

## 14. 开放问题

1. 是否保留旧命令 `STEER_CAL` 等作为别名？建议保留一个版本，打印迁移提示。
2. 是否需要将校准向导拆分为“方向校准”和“油门校准”两个独立入口？本设计采用统一入口一次性采集两轴；如用户希望分开，可在 UI 层增加“仅校准方向/油门”选项。
3. 是否在采集期间临时禁用 actuator 输出？当前设计不主动禁用，因为用户在校准时通常车辆已 Park；如需更安全，可在 `CENTERING/MINMAX` 状态时强制输出零。

## 15. 结论

采用**方案 C：统一 `JoystickCalibration` 模块**。该方案把方向盘与油门校准合并为“手柄校准”，通过单一状态机和统一 NVS 数据模型，实现：

- Drift Console 两步向导（中位 → 正负最大值）；
- 校准值持久化并开机自动加载；
- 在 `ControlMixer` 中对所有使用 RC 输入的模式应用校准，使手动数据与模型数据在 `[-100, 100]` 上对齐。

下一步：进入 `writing-plans` 阶段，拆分实现任务、依赖顺序与验收标准。
