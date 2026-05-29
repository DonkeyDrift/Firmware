# 转向通道（CH1 Steering）交互式标定功能实施方案

## 背景与问题

由于转向机构的机械装配原因：
1. **中位偏移**：车辆正向前进时的机械中位，并不对应 RC 接收机 CH1 的 1500µs 电气中位。
2. **行程受限**：车轮物理摆动幅度有限，摇杆左右打满时的 PWM 值无法覆盖接收机全行程（~872µs ~ 2113µs）。

当前代码中转向映射使用编译时常量（`RC_STEERING_MIN=872`, `RC_STEERING_MID=1488`, `RC_STEERING_MAX=2113`），且 `process_steering_signal()` 中存在硬编码字面量。这导致实际车辆无法走直线，也无法充分利用摇杆的有效行程。

---

## 设计目标

1. 将转向校准常量改为**运行时变量**，支持从中位向两侧非对称映射。
2. 提供**基于串口命令 + TUI 状态显示**的交互式标定流程，引导用户：
   - 步骤1：保持车辆中位不动，自动采集中位 PWM。
   - 步骤2：在倒计时内来回打满摇杆，自动采集最小/最大 PWM。
3. 标定参数**持久化到 NVS**（Preferences），重启后自动恢复。
4. 保留恢复出厂默认值的能力。
5. **安全**：标定全过程要求 Park Locked，且标定时强制 zero-output。

---

## 数据结构设计

在 `SharedTypes.h` 新增（或 `mus4.ino` 顶部）：

```cpp
struct SteeringCalibration {
    int16_t min_pwm;   // 左满舵实际 PWM（摇杆左打到机械限位）
    int16_t mid_pwm;   // 机械中位实际 PWM（车辆直线行驶）
    int16_t max_pwm;   // 右满舵实际 PWM（摇杆右打到机械限位）
};
```

运行期全局变量（`mus4.ino`）：
```cpp
static SteeringCalibration steer_cal = { RC_STEERING_MIN, RC_STEERING_MID, RC_STEERING_MAX };
static bool steer_cal_enabled = false;  // 是否启用标定（从 NVS 读取）
```

NVS Keys：
```cpp
const char* MUS4_PREF_STEER_MIN_KEY = "str_min";
const char* MUS4_PREF_STEER_MID_KEY = "str_mid";
const char* MUS4_PREF_STEER_MAX_KEY = "str_max";
const char* MUS4_PREF_STEER_CAL_EN_KEY = "str_cal";
```

---

## 映射算法

标定后，将原始 PWM 映射到控制量 [-100, 100] 采用**分段线性映射**（中位两侧非对称）：

```cpp
static int mapSteeringCalibrated(int16_t pwm)
{
    if (pwm < steer_cal.mid_pwm) {
        // 从中位到最小值映射到 0 → -100
        return map(pwm, steer_cal.min_pwm, steer_cal.mid_pwm, -100, 0);
    } else {
        // 从中位到最大值映射到 0 → 100
        return map(pwm, steer_cal.mid_pwm, steer_cal.max_pwm, 0, 100);
    }
}
```

**Clamp 保护**：映射结果仍然经过 `constrain(value, -100, 100)`。

**与现有平滑/滤波的关系**：标定映射在**滤波之后**进行。即：
1. 中断采集原始 PWM
2. 中值滤波 + primary smooth（现有逻辑不变）
3. `pwm_filtered[CH_STEERING]` 输入到 `mapSteeringCalibrated()`
4. 结果写入 `rc_data.steering`，后续 PID/融合逻辑不变

---

## 交互流程设计（状态机）

```cpp
enum SteerCalState {
    STEER_CAL_IDLE,     // 空闲
    STEER_CAL_CENTER,   // 采集中位（倒计时 3s 后自动采样）
    STEER_CAL_MINMAX,   // 采集最大最小（倒计时 5s 内持续采样）
    STEER_CAL_DONE      // 采集完成，等待用户确认保存/重试/放弃
};
```

### 配置入口：串口 + Web Console 统一命令

本项目所有串口命令天然同步到 Web Console。Web Console 的 `/api/cmd` 端点会将用户输入的文本命令通过 `processWirelessConsoleLine()` 转发到与串口完全相同的 `PROCESS_COMMAND_LINE` 宏处理。因此：**在串口添加标定命令后，Web Console 立即支持，无需额外 HTTP API 开发。**

为提升 Web Console 体验，可在 `WIFI_WEB_CONSOLE_HTML` 的快捷按钮行中增加标定相关按钮：
```html
<button onclick="quick('STEER_CAL')">STEER_CAL</button>
<button onclick="quick('CAL_STATUS')">CAL_STATUS</button>
```

### 完整交互时序

```
用户输入（串口或 Web Console）: STEER_CAL
↓
系统检查 Park 是否 Locked（未锁定则拒绝，提示 NACK:PARK_REQUIRED）
↓
进入 STEER_CAL_CENTER
  TUI log: "[CAL] 保持车辆中位，3秒后自动采集..."
  TUI log: "[CAL] 3... 2... 1..."
  倒计时结束后，读取 500ms 内 CH1 滤波后 PWM 的中位数 → steer_cal.mid_pwm
  TUI log: "[CAL] 中位采集完成: 1482"
↓
自动进入 STEER_CAL_MINMAX
  TUI log: "[CAL] 请在5秒内将摇杆左右打满..."
  在 5 秒内持续读取 pwm_filtered[CH_STEERING]，更新 running min/max
  TUI log: "[CAL] 倒计时 5... 4... 3... 2... 1..."
  倒计时结束后:
    steer_cal.min_pwm = 采集到的最小值
    steer_cal.max_pwm = 采集到的最大值
  TUI log: "[CAL] 行程采集完成: min=942 max=1987"
↓
自动进入 STEER_CAL_DONE
  TUI log: "[CAL] 结果: mid=1482 min=942 max=1987"
  TUI log: "[CAL] 发送 CAL_SAVE 保存 / CAL_RETRY 重采 / CAL_ABORT 放弃"
↓
用户输入 CAL_SAVE
  写入 NVS，steer_cal_enabled = true
  返回 STEER_CAL_IDLE
  TUI log: "[CAL] 已保存到 NVS"
↓
用户输入 CAL_RETRY
  回到 STEER_CAL_CENTER，重新采集
↓
用户输入 CAL_ABORT
  丢弃本次采集，恢复之前的 steer_cal（或出厂默认值）
  返回 STEER_CAL_IDLE
```

**命令列表（串口与 Web Console 通用）：**

| 命令 | 阶段 | 说明 |
|------|------|------|
| `STEER_CAL` | 任意 | 启动标定流程（要求 Park Locked） |
| `CAL_SAVE` | STEER_CAL_DONE | 保存当前采集结果到 NVS |
| `CAL_RETRY` | STEER_CAL_DONE | 放弃本次结果，重新采集 |
| `CAL_ABORT` | 任意 | 放弃标定，恢复上一次有效值 |
| `CAL_RESET` | 任意 | 恢复出厂默认值（RC_STEERING_MIN/MID/MAX），并清除 NVS |
| `CAL_STATUS` | 任意 | 打印当前标定参数和启用状态 |

---

## TUI 显示设计

TUI 本身无交互输入能力，但可以通过 `tui.log()` 实时显示标定状态，并在 `drawRC()` 中高亮当前正在标定的通道。

### drawRC() 修改
在 `drawRC()`（`TUI.cpp` 约 line 250 附近）中：
- 如果 `steer_cal_state != STEER_CAL_IDLE`，将 CH1 的显示行用反色/高亮标注。
- 显示当前实时 PWM 值，帮助用户观察摇杆位置。

### drawLog() / tui.log() 使用
标定过程中的所有提示文本通过 `tui.log()` 输出，用户在串口终端或 TUI 屏幕的 log 区域可见。

---

## NVS 存储与初始化

### 加载（setup() 中调用）
```cpp
static void loadSteeringCalibration()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, true)) return;
    steer_cal_enabled = mus4Prefs.getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
    if (steer_cal_enabled) {
        steer_cal.min_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        steer_cal.mid_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        steer_cal.max_pwm = (int16_t)mus4Prefs.getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
    }
    mus4Prefs.end();
}
```

### 保存
```cpp
static bool saveSteeringCalibration()
{
    if (!mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) return false;
    mus4Prefs.putShort(MUS4_PREF_STEER_MIN_KEY, steer_cal.min_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MID_KEY, steer_cal.mid_pwm);
    mus4Prefs.putShort(MUS4_PREF_STEER_MAX_KEY, steer_cal.max_pwm);
    mus4Prefs.putBool(MUS4_PREF_STEER_CAL_EN_KEY, true);
    mus4Prefs.end();
    return true;
}
```

### 恢复出厂
```cpp
static void resetSteeringCalibration()
{
    steer_cal = { RC_STEERING_MIN, RC_STEERING_MID, RC_STEERING_MAX };
    steer_cal_enabled = false;
    if (mus4Prefs.begin(MUS4_PREF_NAMESPACE, false)) {
        mus4Prefs.remove(MUS4_PREF_STEER_MIN_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MID_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_MAX_KEY);
        mus4Prefs.remove(MUS4_PREF_STEER_CAL_EN_KEY);
        mus4Prefs.end();
    }
}
```

---

## 代码修改清单

### 文件 1: `mus4.ino`

1. **新增结构体与变量**（在现有 Preferences keys 附近）
   - `SteeringCalibration steer_cal`
   - `bool steer_cal_enabled`
   - `SteerCalState steer_cal_state`
   - `unsigned long steer_cal_stage_start_ms`
   - `int16_t steer_cal_temp_min`, `steer_cal_temp_max`
   - NVS key 常量

2. **新增函数**
   - `loadSteeringCalibration()` — setup 时调用
   - `saveSteeringCalibration()`
   - `resetSteeringCalibration()`
   - `mapSteeringCalibrated(int16_t pwm)` — 核心映射
   - `startSteerCalibration()`
   - `updateSteerCalibration()` — 在主循环中按状态机时序调用
   - `printCalStatus(Print& out)`

3. **修改现有代码**
   - `setup()`: 末尾调用 `loadSteeringCalibration()`
   - 主循环（`loop()` 中 steering 映射处）：
     ```cpp
     // 替换原有的:
     // car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
     // 改为:
     if (steer_cal_enabled) {
         rc_data.steering = mapSteeringCalibrated(pwm_filtered[CH_STEERING]);
     } else {
         rc_data.steering = map(pwm_filtered[CH_STEERING], RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
     }
     ```
   - `process_steering_signal()`（line ~3274）：将硬编码字面量 `872, 1488, 2113` 替换为 `steer_cal.min_pwm, steer_cal.mid_pwm, steer_cal.max_pwm`
   - `PROCESS_COMMAND_LINE` 宏：添加 `STEER_CAL`, `CAL_SAVE`, `CAL_RETRY`, `CAL_ABORT`, `CAL_RESET`, `CAL_STATUS` 分支
   - `isWirelessCommandAllowed()`: 新命令需要 Park Locked 保护（与 TEST/BENCH 同等级）

4. **主循环时序集成**
   - 在 `loop()` 中传感器读取附近（8ms 间隔）添加 `updateSteerCalibration()` 调用
   - 标定状态机内部通过 `millis()` 倒计时，不阻塞主循环

### 文件 2: `SharedTypes.h`

- 添加 `struct SteeringCalibration` 定义（或保持仅在 `mus4.ino` 中定义，避免改动头文件）

### 文件 3: `TUI.cpp`

- `drawRC()`: 标定高亮显示（可选，增强体验）
- 标定状态可通过 `tui.log()` 输出，无需 TUI 本身修改即可工作

---

## 安全设计

1. **Park 锁定检查**：`STEER_CAL` 命令要求 `car_output.park == PARK_LOCKED`，否则返回 `NACK:PARK_REQUIRED`。
2. **零输出保护**：标定状态机 `STEER_CAL_CENTER` 和 `STEER_CAL_MINMAX` 阶段，主循环中的 throttle/steering 输出被强制置 0（或保持最后有效值但不更新）。通过设置 `steer_cal_state != STEER_CAL_IDLE` 时跳过 `mode_change()` 后的输出更新逻辑实现。
3. **范围校验**：采集到的 min/max 必须满足 `min < mid < max` 且三者间距 > 100µs，否则视为无效，提示用户重试。
4. **无线命令权限**：标定命令归类为"需认证 + Park 锁定"级别，与 `TEST`/`BENCH` 同级。修改 `isWirelessCommandAllowed()` 时同步更新 `wireless_console_policy.py` 和单元测试。

---

## 实施步骤

1. 在 `mus4.ino` 中添加 `SteeringCalibration` 数据结构、NVS keys、加载/保存/重置函数。
2. 修改 steering 映射逻辑，支持 `steer_cal_enabled` 条件分支。
3. 实现标定状态机 `updateSteerCalibration()` 和串口命令响应。
4. 修改 `process_steering_signal()` 中的硬编码字面量。
5. 在 `isWirelessCommandAllowed()` 中添加新命令权限，并同步 `wireless_console_policy.py`。
6. 添加单元测试到 `tests/`（如有需要）。
7. 更新 `AGENTS.md` 中相关命令列表。
8. 编译验证并实际测试标定流程。

---

## 预期效果

标定完成后，用户摇杆的有效行程（机械限位对应的 PWM 范围）将被完整映射到 [-100, 100] 控制量，中位精确对应车辆直线行驶状态。即使中位偏移、左右行程不对称，也能获得线性且满幅的转向控制。
