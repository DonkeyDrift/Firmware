# 手柄/摇杆校准实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 MUS4_FW 中实现统一的手柄（方向 + 油门）零位与正负最大值校准，包括 NVS 持久化、Drift Console 向导 UI、全模式输出校准，并安全迁移旧方向盘校准数据。

**Architecture:** 用新的 `JoystickCalibration` 模块替换现有 `SteeringCalibration`，同时处理两轴；在 `ControlMixer` 的 RC→归一化阶段统一应用校准映射；通过 `/api/cmd` 暴露命令，并在 `WebConsoleAssets.h` 中新增模态向导。

**Tech Stack:** C++17/Arduino framework, ESP32 Preferences (NVS), Python/pytest, HTML/JS/CSS (PROGMEM).

---

## 文件结构

| 文件 | 动作 | 职责 |
|---|---|---|
| `MUS4_FW/libraries/mus4_control/src/JoystickCalibration.h` | 新建 | 公共 API、数据结构、状态机枚举、映射函数声明 |
| `MUS4_FW/libraries/mus4_control/src/JoystickCalibration.cpp` | 新建 | NVS 读写（含迁移）、采集状态机、映射实现 |
| `MUS4_FW/libraries/mus4_control/src/SteeringCalibration.h` | 删除 | 功能合并到 `JoystickCalibration` |
| `MUS4_FW/libraries/mus4_control/src/SteeringCalibration.cpp` | 删除 | 功能合并到 `JoystickCalibration` |
| `MUS4_FW/libraries/mus4_core/src/WifiConsoleTypes.h` | 修改 | 新增 8 个 joystick NVS key 常量 |
| `MUS4_FW/libraries/mus4_control/src/ControlMixer.cpp` | 修改 | 使用 `mapJoystickAxis` 替换方向/油门映射 |
| `MUS4_FW/libraries/mus4_control/src/SteeringControl.cpp` | 修改 | 使用 `joystick_cal.steering` 替换 `steer_cal` |
| `MUS4_FW/libraries/mus4_control/src/mus4_control.h` | 修改 | 聚合头文件包含 `JoystickCalibration.h` |
| `MUS4_FW/MUS4_FW.ino` | 修改 | 启动时调用 `loadJoystickCalibration()` |
| `MUS4_FW/libraries/mus4_command/src/CommandDispatcher.cpp` | 修改 | 替换 `STEER_CAL/CAL_*` 为 `JOYSTICK_CAL/JOYSTICK_*` |
| `MUS4_FW/libraries/mus4_command/src/WirelessConsole.cpp` | 修改 | 同步 Park-locked 命令分类 |
| `MUS4_FW/wireless_console_policy.py` | 修改 | 同步 Park-locked 命令集合 |
| `MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp` | 修改（可选） | 新增 `/api/joystick_cal` JSON 端点 |
| `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h` | 修改 | 新增校准按钮、状态显示、模态向导 |
| `MUS4_FW/tests/test_joystick_calibration.py` | 新建 | 映射函数与状态机单元测试 |
| `MUS4_FW/tests/test_wireless_console_policy.py` | 修改 | 新增新命令权限测试 |

---

## Task 1: 新增 NVS key 常量

**Files:**
- Modify: `MUS4_FW/libraries/mus4_core/src/WifiConsoleTypes.h:49-52`

- [ ] **Step 1: 在原有 steering key 下方新增 joystick key**

```cpp
// --- Joystick calibration keys (unified steering + throttle) ---
static const char* MUS4_PREF_JOYSTICK_STEER_MIN_KEY = "js_st_min";
static const char* MUS4_PREF_JOYSTICK_STEER_MID_KEY = "js_st_mid";
static const char* MUS4_PREF_JOYSTICK_STEER_MAX_KEY = "js_st_max";
static const char* MUS4_PREF_JOYSTICK_STEER_EN_KEY  = "js_st_en";

static const char* MUS4_PREF_JOYSTICK_THROT_MIN_KEY = "js_th_min";
static const char* MUS4_PREF_JOYSTICK_THROT_MID_KEY = "js_th_mid";
static const char* MUS4_PREF_JOYSTICK_THROT_MAX_KEY = "js_th_max";
static const char* MUS4_PREF_JOYSTICK_THROT_EN_KEY  = "js_th_en";
```

- [ ] **Step 2: 编译检查 key 文件**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: 可能因 `JoystickCalibration` 尚未存在而失败，但 `WifiConsoleTypes.h` 本身应无错误。

- [ ] **Step 3: Commit**

```bash
git add MUS4_FW/libraries/mus4_core/src/WifiConsoleTypes.h
git commit -m "feat(joystick-cal): add NVS key constants"
```

---

## Task 2: 创建 JoystickCalibration 头文件

**Files:**
- Create: `MUS4_FW/libraries/mus4_control/src/JoystickCalibration.h`

- [ ] **Step 1: 写入头文件**

```cpp
#pragma once
#include <Arduino.h>
#include "SharedTypes.h"
#include "RuntimeState.h"

void setJoystickCalibrationRuntimeState(WifiRuntimeState& ws);

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

enum class JoystickCalState {
    IDLE,
    CENTERING,
    MINMAX,
    DONE
};

extern JoystickCalibrationData joystick_cal;
extern JoystickCalState joystick_cal_state;
extern unsigned long joystick_cal_stage_start_ms;
extern int16_t joystick_cal_temp_min[2];
extern int16_t joystick_cal_temp_max[2];

void loadJoystickCalibration();
bool saveJoystickCalibration();
void resetJoystickCalibration();

int mapJoystickAxis(int16_t pwm,
                    const AxisCalibration& cal,
                    bool enabled,
                    int16_t default_min,
                    int16_t default_mid,
                    int16_t default_max);

// 临时兼容别名：旧代码/测试可能仍引用 mapSteeringCalibrated
inline int mapSteeringCalibrated(int16_t pwm) {
    return mapJoystickAxis(pwm, joystick_cal.steering, joystick_cal.steering_enabled,
                           RC_STEERING_MIN, RC_STEERING_MID, RC_STEERING_MAX);
}

void printJoystickCalStatus(Print& out);
bool startJoystickCalibration(Print& out);
void updateJoystickCalibration();
void abortJoystickCalibration();
bool validateJoystickCalibration(const AxisCalibration& axis);
```

- [ ] **Step 2: Commit**

```bash
git add MUS4_FW/libraries/mus4_control/src/JoystickCalibration.h
git commit -m "feat(joystick-cal): add JoystickCalibration header"
```

---

## Task 3: 创建 JoystickCalibration 实现文件

**Files:**
- Create: `MUS4_FW/libraries/mus4_control/src/JoystickCalibration.cpp`

- [ ] **Step 1: 写入实现骨架**

```cpp
#include "JoystickCalibration.h"

#include <Preferences.h>
#include <limits.h>

#include "FirmwareConfig.h"
#include "Mus4Log.h"
#include "SharedTypes.h"
#include "TUI.h"
#include "WifiConsoleTypes.h"

extern TUI tui;
extern ControlData car_output;
extern uint16_t pwm_filtered[];

static WifiRuntimeState* g_ws = nullptr;

void setJoystickCalibrationRuntimeState(WifiRuntimeState& ws)
{
    g_ws = &ws;
}

static inline Preferences& prefs()
{
    return *g_ws->prefs;
}

JoystickCalibrationData joystick_cal;
JoystickCalState joystick_cal_state = JoystickCalState::IDLE;
unsigned long joystick_cal_stage_start_ms = 0;
int16_t joystick_cal_temp_min[2] = {0, 0};
int16_t joystick_cal_temp_max[2] = {0, 0};

int mapJoystickAxis(int16_t pwm,
                    const AxisCalibration& cal,
                    bool enabled,
                    int16_t default_min,
                    int16_t default_mid,
                    int16_t default_max)
{
    if (!enabled) {
        int v = map(pwm, default_min, default_max, -100, 100);
        return constrain(v, -100, 100);
    }

    if (pwm < cal.mid_pwm) {
        int v = map(pwm, cal.min_pwm, cal.mid_pwm, -100, 0);
        return constrain(v, -100, 0);
    }
    int v = map(pwm, cal.mid_pwm, cal.max_pwm, 0, 100);
    return constrain(v, 0, 100);
}

bool validateJoystickCalibration(const AxisCalibration& axis)
{
    return axis.min_pwm < axis.mid_pwm && axis.mid_pwm < axis.max_pwm
        && (axis.mid_pwm - axis.min_pwm) > 100
        && (axis.max_pwm - axis.mid_pwm) > 100
        && axis.min_pwm >= RC_PWM_MIN
        && axis.max_pwm <= RC_PWM_MAX;
}

void loadJoystickCalibration()
{
    joystick_cal.steering.min_pwm = RC_STEERING_MIN;
    joystick_cal.steering.mid_pwm = RC_STEERING_MID;
    joystick_cal.steering.max_pwm = RC_STEERING_MAX;
    joystick_cal.throttle.min_pwm = RC_THROTTLE_MIN;
    joystick_cal.throttle.mid_pwm = RC_THROTTLE_MID;
    joystick_cal.throttle.max_pwm = RC_THROTTLE_MAX;

    if (!prefs().begin(MUS4_PREF_NAMESPACE, true)) {
        joystick_cal.steering_enabled = false;
        joystick_cal.throttle_enabled = false;
        return;
    }

    // Steering: prefer new keys; fall back to legacy keys once.
    bool migrated = false;
    if (prefs().isKey(MUS4_PREF_JOYSTICK_STEER_MIN_KEY)) {
        joystick_cal.steering.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MIN_KEY, RC_STEERING_MIN);
        joystick_cal.steering.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MID_KEY, RC_STEERING_MID);
        joystick_cal.steering.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_STEER_MAX_KEY, RC_STEERING_MAX);
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_STEER_EN_KEY, false);
    } else if (prefs().isKey(MUS4_PREF_STEER_MIN_KEY)) {
        joystick_cal.steering.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MIN_KEY, RC_STEERING_MIN);
        joystick_cal.steering.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MID_KEY, RC_STEERING_MID);
        joystick_cal.steering.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_STEER_MAX_KEY, RC_STEERING_MAX);
        joystick_cal.steering_enabled = prefs().getBool(MUS4_PREF_STEER_CAL_EN_KEY, false);
        migrated = true;
    }

    joystick_cal.throttle.min_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MIN_KEY, RC_THROTTLE_MIN);
    joystick_cal.throttle.mid_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MID_KEY, RC_THROTTLE_MID);
    joystick_cal.throttle.max_pwm = (int16_t)prefs().getShort(MUS4_PREF_JOYSTICK_THROT_MAX_KEY, RC_THROTTLE_MAX);
    joystick_cal.throttle_enabled = prefs().getBool(MUS4_PREF_JOYSTICK_THROT_EN_KEY, false);

    prefs().end();

    if (migrated) {
        saveJoystickCalibration();
        mus4LogLine("cal", "migrated legacy steering calibration");
    }

    mus4Logf("cal", "joystick steer_en=%d throttle_en=%d",
             joystick_cal.steering_enabled ? 1 : 0,
             joystick_cal.throttle_enabled ? 1 : 0);
}

bool saveJoystickCalibration()
{
    if (!prefs().begin(MUS4_PREF_NAMESPACE, false)) return false;

    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MIN_KEY, joystick_cal.steering.min_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MID_KEY, joystick_cal.steering.mid_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_STEER_MAX_KEY, joystick_cal.steering.max_pwm);
    prefs().putBool(MUS4_PREF_JOYSTICK_STEER_EN_KEY, joystick_cal.steering_enabled);

    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MIN_KEY, joystick_cal.throttle.min_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MID_KEY, joystick_cal.throttle.mid_pwm);
    prefs().putShort(MUS4_PREF_JOYSTICK_THROT_MAX_KEY, joystick_cal.throttle.max_pwm);
    prefs().putBool(MUS4_PREF_JOYSTICK_THROT_EN_KEY, joystick_cal.throttle_enabled);

    prefs().end();
    mus4Logf("cal", "saved steer min=%d mid=%d max=%d; throttle min=%d mid=%d max=%d",
             joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
             joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm);
    return true;
}

void resetJoystickCalibration()
{
    joystick_cal.steering.min_pwm = RC_STEERING_MIN;
    joystick_cal.steering.mid_pwm = RC_STEERING_MID;
    joystick_cal.steering.max_pwm = RC_STEERING_MAX;
    joystick_cal.steering_enabled = false;
    joystick_cal.throttle.min_pwm = RC_THROTTLE_MIN;
    joystick_cal.throttle.mid_pwm = RC_THROTTLE_MID;
    joystick_cal.throttle.max_pwm = RC_THROTTLE_MAX;
    joystick_cal.throttle_enabled = false;

    if (prefs().begin(MUS4_PREF_NAMESPACE, false)) {
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MIN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MID_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_MAX_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_STEER_EN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MIN_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MID_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_MAX_KEY);
        prefs().remove(MUS4_PREF_JOYSTICK_THROT_EN_KEY);
        prefs().end();
    }
    mus4LogLine("cal", "reset to defaults");
}

void printJoystickCalStatus(Print& out)
{
    out.printf("JOYSTICK_STATUS steer_en=%d st_min=%d st_mid=%d st_max=%d "
               "throttle_en=%d th_min=%d th_mid=%d th_max=%d state=%d\n",
               joystick_cal.steering_enabled ? 1 : 0,
               joystick_cal.steering.min_pwm, joystick_cal.steering.mid_pwm, joystick_cal.steering.max_pwm,
               joystick_cal.throttle_enabled ? 1 : 0,
               joystick_cal.throttle.min_pwm, joystick_cal.throttle.mid_pwm, joystick_cal.throttle.max_pwm,
               (int)joystick_cal_state);
}

bool startJoystickCalibration(Print& out)
{
    if (car_output.park != PARK_LOCKED) {
        out.println("NACK:PARK_REQUIRED");
        return false;
    }
    joystick_cal_state = JoystickCalState::CENTERING;
    joystick_cal_stage_start_ms = millis();
    joystick_cal_temp_min[0] = INT16_MAX;
    joystick_cal_temp_min[1] = INT16_MAX;
    joystick_cal_temp_max[0] = INT16_MIN;
    joystick_cal_temp_max[1] = INT16_MIN;
    tui.log("[CAL] Step 1/2: keep joystick centered (throttle + steering)");
    mus4LogLine("cal", "centering stage started");
    return true;
}

void abortJoystickCalibration()
{
    joystick_cal_state = JoystickCalState::IDLE;
    loadJoystickCalibration();
    mus4LogLine("cal", "aborted");
}

static void captureCenter()
{
    joystick_cal.steering.mid_pwm = (int16_t)pwm_filtered[CH_STEERING];
    joystick_cal.throttle.mid_pwm = (int16_t)pwm_filtered[CH_THROTTLE];
    char buf[96];
    snprintf(buf, sizeof(buf), "[CAL] Center captured: st=%d thr=%d",
             joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);
    tui.log(buf);
    mus4Logf("cal", "center st=%d thr=%d",
             joystick_cal.steering.mid_pwm, joystick_cal.throttle.mid_pwm);
}

static void captureMinMax()
{
    joystick_cal.steering.min_pwm = joystick_cal_temp_min[0];
    joystick_cal.steering.max_pwm = joystick_cal_temp_max[0];
    joystick_cal.throttle.min_pwm = joystick_cal_temp_min[1];
    joystick_cal.throttle.max_pwm = joystick_cal_temp_max[1];
    char buf[128];
    snprintf(buf, sizeof(buf),
             "[CAL] Range captured: st_min=%d st_max=%d thr_min=%d thr_max=%d",
             joystick_cal.steering.min_pwm, joystick_cal.steering.max_pwm,
             joystick_cal.throttle.min_pwm, joystick_cal.throttle.max_pwm);
    tui.log(buf);
    mus4Logf("cal", "range st=%d/%d thr=%d/%d",
             joystick_cal.steering.min_pwm, joystick_cal.steering.max_pwm,
             joystick_cal.throttle.min_pwm, joystick_cal.throttle.max_pwm);
}

void updateJoystickCalibration()
{
    if (joystick_cal_state == JoystickCalState::IDLE) return;

    unsigned long now = millis();
    unsigned long elapsed = now - joystick_cal_stage_start_ms;

    if (joystick_cal_state == JoystickCalState::CENTERING) {
        if (elapsed < 3000) return;
        captureCenter();
        joystick_cal_state = JoystickCalState::MINMAX;
        joystick_cal_stage_start_ms = now;
        joystick_cal_temp_min[0] = INT16_MAX;
        joystick_cal_temp_min[1] = INT16_MAX;
        joystick_cal_temp_max[0] = INT16_MIN;
        joystick_cal_temp_max[1] = INT16_MIN;
        tui.log("[CAL] Step 2/2: move stick to all extremes within 5s");
    } else if (joystick_cal_state == JoystickCalState::MINMAX) {
        int16_t st = (int16_t)pwm_filtered[CH_STEERING];
        int16_t th = (int16_t)pwm_filtered[CH_THROTTLE];
        if (st < joystick_cal_temp_min[0]) joystick_cal_temp_min[0] = st;
        if (st > joystick_cal_temp_max[0]) joystick_cal_temp_max[0] = st;
        if (th < joystick_cal_temp_min[1]) joystick_cal_temp_min[1] = th;
        if (th > joystick_cal_temp_max[1]) joystick_cal_temp_max[1] = th;
        if (elapsed < 5000) return;
        captureMinMax();
        joystick_cal_state = JoystickCalState::DONE;
        tui.log("[CAL] Done. Send JOYSTICK_SAVE / JOYSTICK_RETRY / JOYSTICK_ABORT");
    }
}
```

- [ ] **Step 2: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: 可能因 `ControlMixer`/`CommandDispatcher` 仍引用旧头文件而失败，但 `JoystickCalibration.cpp` 本身应无错误。

- [ ] **Step 3: Commit**

```bash
git add MUS4_FW/libraries/mus4_control/src/JoystickCalibration.cpp
git commit -m "feat(joystick-cal): add JoystickCalibration implementation"
```

---

## Task 4: 在 MUS4_FW.ino 中加载新的校准模块

**Files:**
- Modify: `MUS4_FW/MUS4_FW.ino:56`
- Modify: `MUS4_FW/MUS4_FW.ino:346`
- Modify: `MUS4_FW/MUS4_FW.ino:375`

- [ ] **Step 1: 替换头文件包含**

找到约 line 56：
```cpp
#include "SteeringCalibration.h"
```
替换为：
```cpp
#include "JoystickCalibration.h"
```

- [ ] **Step 2: 替换 runtime state 绑定**

找到 `setup()` 中约 line 346：
```cpp
setSteeringCalibrationRuntimeState(wifiRuntime);
```
替换为：
```cpp
setJoystickCalibrationRuntimeState(wifiRuntime);
```

- [ ] **Step 3: 替换启动加载调用**

找到约 line 375：
```cpp
loadSteeringCalibration();
```
替换为：
```cpp
loadJoystickCalibration();
```

- [ ] **Step 4: 移除旧的 steering calibration 前向声明**

搜索 `loadSteeringCalibration` 与 `setSteeringCalibrationRuntimeState` 引用并删除。

- [ ] **Step 4: Commit**

```bash
git add MUS4_FW/MUS4_FW.ino
git commit -m "feat(joystick-cal): load JoystickCalibration at boot"
```

---

## Task 5: 更新 ControlMixer 使用新映射

**Files:**
- Modify: `MUS4_FW/libraries/mus4_control/src/ControlMixer.cpp`

- [ ] **Step 1: 替换头文件与 extern 声明**

将：
```cpp
#include "SteeringCalibration.h"
```
替换为：
```cpp
#include "JoystickCalibration.h"
```

将：
```cpp
extern bool steer_cal_enabled;
```
替换为：
```cpp
extern JoystickCalibrationData joystick_cal;
```

- [ ] **Step 2: 替换油门与方向映射**

将 SEMI_AUTO 分支中的：
```cpp
car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
```
替换为：
```cpp
car_output.throttle = mapJoystickAxis(rc_data.throttle,
                                      joystick_cal.throttle,
                                      joystick_cal.throttle_enabled,
                                      RC_THROTTLE_MIN,
                                      RC_THROTTLE_MID,
                                      RC_THROTTLE_MAX);
```

将 MANUAL 分支中的：
```cpp
car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
```
替换为同样的 `mapJoystickAxis(...throttle...)` 调用。

将 MANUAL 分支中的：
```cpp
if (steer_cal_enabled) {
    car_output.steering = mapSteeringCalibrated(rc_data.steering);
} else {
    car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
}
```
替换为：
```cpp
car_output.steering = mapJoystickAxis(rc_data.steering,
                                      joystick_cal.steering,
                                      joystick_cal.steering_enabled,
                                      RC_STEERING_MIN,
                                      RC_STEERING_MID,
                                      RC_STEERING_MAX);
```

- [ ] **Step 3: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS（此时旧 `SteeringCalibration` 文件仍存在，但不影响）

- [ ] **Step 4: Commit**

```bash
git add MUS4_FW/libraries/mus4_control/src/ControlMixer.cpp
git commit -m "feat(joystick-cal): apply calibrated mapping in ControlMixer"
```

---

## Task 6: 更新命令分发器

**Files:**
- Modify: `MUS4_FW/libraries/mus4_command/src/CommandDispatcher.cpp`

- [ ] **Step 1: 替换头文件包含**

将：
```cpp
#include "SteeringCalibration.h"
```
替换为：
```cpp
#include "JoystickCalibration.h"
```

- [ ] **Step 2: 替换校准命令处理**

删除旧 `STEER_CAL`、`CAL_SAVE`、`CAL_RETRY`、`CAL_ABORT`、`CAL_RESET`、`CAL_STATUS` 代码块，替换为：

```cpp
    if (line.equalsIgnoreCase("JOYSTICK_CAL")) {
        startJoystickCalibration(out);
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_SAVE")) {
        if (joystick_cal_state == JoystickCalState::DONE) {
            bool steer_ok = validateJoystickCalibration(joystick_cal.steering);
            bool thr_ok = validateJoystickCalibration(joystick_cal.throttle);
            if (steer_ok && thr_ok) {
                joystick_cal.steering_enabled = true;
                joystick_cal.throttle_enabled = true;
                if (saveJoystickCalibration()) {
                    joystick_cal_state = JoystickCalState::IDLE;
                    out.println("ACK:JOYSTICK_SAVED");
                } else {
                    out.println("NACK:JOYSTICK_SAVE_FAILED");
                }
            } else {
                out.printf("NACK:JOYSTICK_INVALID_RANGE steer_ok=%d thr_ok=%d\n", steer_ok, thr_ok);
            }
        } else {
            out.println("NACK:JOYSTICK_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_RETRY")) {
        if (joystick_cal_state == JoystickCalState::DONE || joystick_cal_state == JoystickCalState::MINMAX) {
            joystick_cal_state = JoystickCalState::CENTERING;
            joystick_cal_stage_start_ms = millis();
            joystick_cal_temp_min[0] = INT16_MAX;
            joystick_cal_temp_min[1] = INT16_MAX;
            joystick_cal_temp_max[0] = INT16_MIN;
            joystick_cal_temp_max[1] = INT16_MIN;
            tui.log("[CAL] Retrying from center capture...");
            out.println("ACK:JOYSTICK_RETRY");
        } else {
            out.println("NACK:JOYSTICK_NOT_DONE");
        }
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_ABORT")) {
        abortJoystickCalibration();
        out.println("ACK:JOYSTICK_ABORTED");
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_RESET")) {
        resetJoystickCalibration();
        joystick_cal_state = JoystickCalState::IDLE;
        out.println("ACK:JOYSTICK_RESET");
        return true;
    }
    if (line.equalsIgnoreCase("JOYSTICK_STATUS")) {
        printJoystickCalStatus(out);
        return true;
    }

    // 旧命令兼容别名，保留一个版本
    if (line.equalsIgnoreCase("STEER_CAL")) {
        out.println("ACK:DEPRECATED_USE_JOYSTICK_CAL");
        return startJoystickCalibration(out);
    }
    if (line.equalsIgnoreCase("CAL_STATUS")) {
        printJoystickCalStatus(out);
        return true;
    }
```

- [ ] **Step 3: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add MUS4_FW/libraries/mus4_command/src/CommandDispatcher.cpp
git commit -m "feat(joystick-cal): add JOYSTICK_* commands, keep STEER_CAL alias"
```

---

## Task 7: 同步无线控制台权限

**Files:**
- Modify: `MUS4_FW/libraries/mus4_command/src/WirelessConsole.cpp:117-131`

- [ ] **Step 1: 扩展 isParkLockedWirelessCommand**

在原有 `STEER_CAL`/`CAL_*` 列表后新增新命令（**不包含** `JOYSTICK_STATUS`）：

```cpp
        line.equalsIgnoreCase("JOYSTICK_CAL") ||
        line.equalsIgnoreCase("JOYSTICK_SAVE") ||
        line.equalsIgnoreCase("JOYSTICK_RETRY") ||
        line.equalsIgnoreCase("JOYSTICK_ABORT") ||
        line.equalsIgnoreCase("JOYSTICK_RESET") ||
```

- [ ] **Step 2: 将 JOYSTICK_STATUS 加入普通认证命令白名单**

在 `isWirelessCommandAllowed` 中，把 `JOYSTICK_STATUS` 加到 DEV ON 显式白名单那一行（与 `LOG_WEB`、`LOG_SERIAL` 同级），使其仅需认证、无需 Park：

```cpp
if (line.equalsIgnoreCase("ANSI") || line.equalsIgnoreCase("NOANSI") || line.equalsIgnoreCase("FILTER_DEBUG") || line.equalsIgnoreCase("LOG_WEB") || line.equalsIgnoreCase("LOG_SERIAL") || line.equalsIgnoreCase("JOYSTICK_STATUS") || isWifiStaConfigCommand(line)) return ws.consoleAuthenticated || webDevMode;
```

- [ ] **Step 3: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add MUS4_FW/libraries/mus4_command/src/WirelessConsole.cpp
git commit -m "feat(joystick-cal): classify joystick commands; status is auth-only"
```

---

## Task 8: 更新 Python 无线策略

**Files:**
- Modify: `MUS4_FW/wireless_console_policy.py:2`

- [ ] **Step 1: 扩展 PARK_LOCKED_COMMANDS 并移动 JOYSTICK_STATUS**

```python
GENERAL_AUTHENTICATED_COMMANDS = {"ANSI", "NOANSI", "FILTER_DEBUG", "LOG_WEB", "LOG_SERIAL", "JOYSTICK_STATUS"}
PARK_LOCKED_COMMANDS = {
    "TEST", "TEST_TUI", "BENCH", "STRESS", "REGRESS", "FILTER_TEST",
    # legacy steering calibration aliases
    "STEER_CAL", "CAL_SAVE", "CAL_RETRY", "CAL_ABORT", "CAL_RESET", "CAL_STATUS",
    # unified joystick calibration
    "JOYSTICK_CAL", "JOYSTICK_SAVE", "JOYSTICK_RETRY",
    "JOYSTICK_ABORT", "JOYSTICK_RESET",
}
```

- [ ] **Step 2: Commit**

```bash
git add MUS4_FW/wireless_console_policy.py
git commit -m "feat(joystick-cal): status auth-only, rest park-locked"
```

---

## Task 9: 更新剩余 SteeringCalibration 引用并删除旧文件

**Files:**
- Modify: `MUS4_FW/libraries/mus4_control/src/mus4_control.h:16`
- Modify: `MUS4_FW/libraries/mus4_control/src/SteeringControl.cpp:5,82-84`
- Delete: `MUS4_FW/libraries/mus4_control/src/SteeringCalibration.h`
- Delete: `MUS4_FW/libraries/mus4_control/src/SteeringCalibration.cpp`

- [ ] **Step 1: 更新 mus4_control.h 聚合头文件**

将：
```cpp
#include "SteeringCalibration.h"
```
替换为：
```cpp
#include "JoystickCalibration.h"
```

- [ ] **Step 2: 更新 SteeringControl.cpp**

替换头文件包含：
```cpp
#include "JoystickCalibration.h"
```

替换变量引用（约 line 82-84）：
```cpp
    int16_t cal_mid = joystick_cal.steering_enabled ? joystick_cal.steering.mid_pwm : RC_STEERING_MID;
    int16_t cal_min = joystick_cal.steering_enabled ? joystick_cal.steering.min_pwm : RC_STEERING_MIN;
    int16_t cal_max = joystick_cal.steering_enabled ? joystick_cal.steering.max_pwm : RC_STEERING_MAX;
```

- [ ] **Step 3: 删除旧文件**

```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
rm libraries/mus4_control/src/SteeringCalibration.h
rm libraries/mus4_control/src/SteeringCalibration.cpp
```

- [ ] **Step 4: 全局搜索残留引用**

```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
grep -R "SteeringCalibration\|steer_cal_state\|steer_cal_temp_\|steer_cal_enabled" --include="*.cpp" --include="*.h" --include="*.ino" --include="*.py" .
```
Expected: 无匹配（`mapSteeringCalibrated` 别名保留在 `JoystickCalibration.h` 中，不应出现在 grep 结果中）。

- [ ] **Step 5: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add MUS4_FW/libraries/mus4_control/src/mus4_control.h
git add MUS4_FW/libraries/mus4_control/src/SteeringControl.cpp
git rm MUS4_FW/libraries/mus4_control/src/SteeringCalibration.h
git rm MUS4_FW/libraries/mus4_control/src/SteeringCalibration.cpp
git commit -m "refactor(joystick-cal): remove legacy SteeringCalibration files"
```

---

## Task 10: 新增映射函数单元测试

**Files:**
- Create: `MUS4_FW/tests/test_joystick_calibration.py`

- [ ] **Step 1: 写入测试文件**

```python
"""Unit tests for the joystick calibration mapping logic.

These tests mirror the Arduino implementation in Python so they can run
without flashing hardware. Keep them in sync with JoystickCalibration.cpp.
"""

import pathlib
import sys
import unittest


def map_joystick_axis(pwm, cal, enabled, default_min, default_mid, default_max):
    """Python mirror of mapJoystickAxis()."""
    if not enabled:
        v = int((pwm - default_min) * 200 / (default_max - default_min)) - 100
        return max(-100, min(100, v))

    if pwm < cal["mid_pwm"]:
        v = int((pwm - cal["min_pwm"]) * 100 / (cal["mid_pwm"] - cal["min_pwm"])) - 100
        return max(-100, min(0, v))
    v = int((pwm - cal["mid_pwm"]) * 100 / (cal["max_pwm"] - cal["mid_pwm"]))
    return max(0, min(100, v))


def validate_axis(axis):
    return (
        axis["min_pwm"] < axis["mid_pwm"] < axis["max_pwm"]
        and (axis["mid_pwm"] - axis["min_pwm"]) > 100
        and (axis["max_pwm"] - axis["mid_pwm"]) > 100
        and axis["min_pwm"] >= 800
        and axis["max_pwm"] <= 2200
    )


class TestJoystickCalibrationMapping(unittest.TestCase):
    def test_center_returns_zero(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(1500, cal, True, 1000, 1500, 2000), 0)

    def test_min_returns_negative_100(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(1000, cal, True, 1000, 1500, 2000), -100)

    def test_max_returns_100(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(2000, cal, True, 1000, 1500, 2000), 100)

    def test_below_min_clamps(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(500, cal, True, 1000, 1500, 2000), -100)

    def test_above_max_clamps(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(2500, cal, True, 1000, 1500, 2000), 100)

    def test_disabled_uses_defaults(self):
        cal = {"min_pwm": 900, "mid_pwm": 1400, "max_pwm": 1900}
        self.assertEqual(map_joystick_axis(1500, cal, False, 1000, 1500, 2000), 0)
        self.assertEqual(map_joystick_axis(1000, cal, False, 1000, 1500, 2000), -100)
        self.assertEqual(map_joystick_axis(2000, cal, False, 1000, 1500, 2000), 100)

    def test_validation_accepts_reasonable_range(self):
        self.assertTrue(validate_axis({"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}))

    def test_validation_rejects_min_equal_mid(self):
        self.assertFalse(validate_axis({"min_pwm": 1500, "mid_pwm": 1500, "max_pwm": 2000}))

    def test_validation_rejects_too_narrow_range(self):
        self.assertFalse(validate_axis({"min_pwm": 1490, "mid_pwm": 1500, "max_pwm": 1510}))

    def test_validation_rejects_out_of_pwm_bounds(self):
        self.assertFalse(validate_axis({"min_pwm": 700, "mid_pwm": 1500, "max_pwm": 2000}))
        self.assertFalse(validate_axis({"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2300}))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: 运行测试**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
pytest tests/test_joystick_calibration.py -v
```
Expected: 10 tests PASS

- [ ] **Step 3: Commit**

```bash
git add MUS4_FW/tests/test_joystick_calibration.py
git commit -m "test(joystick-cal): add mapping and validation unit tests"
```

---

## Task 11: 更新无线策略测试

**Files:**
- Modify: `MUS4_FW/tests/test_wireless_console_policy.py`

- [ ] **Step 1: 新增测试用例**

在 `test_requires_park_locked_for_diagnostic_commands` 附近新增：

```python
    def test_requires_park_locked_for_joystick_calibration_commands(self):
        for cmd in [
            "JOYSTICK_CAL", "JOYSTICK_SAVE", "JOYSTICK_RETRY",
            "JOYSTICK_ABORT", "JOYSTICK_RESET",
        ]:
            with self.subTest(cmd=cmd):
                self.assertFalse(POLICY.is_wireless_command_allowed(cmd, authenticated=True, park_locked=False))
                self.assertTrue(POLICY.is_wireless_command_allowed(cmd, authenticated=True, park_locked=True))

    def test_joystick_status_requires_authentication_not_park(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("JOYSTICK_STATUS", authenticated=False, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("JOYSTICK_STATUS", authenticated=True, park_locked=False))
        self.assertTrue(POLICY.is_wireless_command_allowed("JOYSTICK_STATUS", authenticated=True, park_locked=True))

    def test_joystick_cal_rejects_unauthenticated(self):
        self.assertFalse(POLICY.is_wireless_command_allowed("JOYSTICK_CAL", authenticated=False, park_locked=True))
```

- [ ] **Step 2: 运行策略测试**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
pytest tests/test_wireless_console_policy.py -v
```
Expected: All PASS

- [ ] **Step 3: Commit**

```bash
git add MUS4_FW/tests/test_wireless_console_policy.py
git commit -m "test(joystick-cal): verify park-locked policy for new commands"
```

---

## Task 12: 添加 Web Console JSON 端点（可选但推荐）

**Files:**
- Modify: `MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp`

- [ ] **Step 1: 新增端点处理函数**

在文件顶部确认包含：
```cpp
#include "JoystickCalibration.h"
```

在 `handleWifiWebDevMode` 附近新增：

```cpp
static void handleWifiWebJoystickCal()
{
    String response;
    StringPrint out(response);
    // JOYSTICK_STATUS 已设为仅需认证，复用无线控制台权限检查。
    processWirelessConsoleLine("JOYSTICK_STATUS", out, WIRELESS_ORIGIN_WEB);
    wifiWebServer.send(200, "text/plain", response);
}

static void handleWifiWebJoystickCalSet()
{
    String action = wifiWebServer.arg("action");
    String cmd;
    if (action.equalsIgnoreCase("start")) cmd = "JOYSTICK_CAL";
    else if (action.equalsIgnoreCase("save")) cmd = "JOYSTICK_SAVE";
    else if (action.equalsIgnoreCase("retry")) cmd = "JOYSTICK_RETRY";
    else if (action.equalsIgnoreCase("abort")) cmd = "JOYSTICK_ABORT";
    else if (action.equalsIgnoreCase("reset")) cmd = "JOYSTICK_RESET";

    String response;
    StringPrint out(response);
    if (cmd.length() > 0) {
        processWirelessConsoleLine(cmd, out, WIRELESS_ORIGIN_WEB);
    } else {
        out.println("NACK:UNKNOWN_ACTION");
    }
    wifiWebServer.send(200, "text/plain", response);
}
```

- [ ] **Step 2: 注册路由**

在 `setupWebConsoleServer()` 中添加：

```cpp
wifiWebServer.on("/api/joystick-cal", HTTP_GET, handleWifiWebJoystickCal);
wifiWebServer.on("/api/joystick-cal", HTTP_POST, handleWifiWebJoystickCalSet);
```

- [ ] **Step 3: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 4: Commit**

```bash
git add MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp
git commit -m "feat(joystick-cal): add /api/joystick-cal endpoint"
```

---

## Task 13: 更新 Drift Console UI

**Files:**
- Modify: `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`

- [ ] **Step 1: 在 RC Channels 面板旁新增入口按钮**

在 `rcFold` 结束后的合适位置插入：

```html
<div style="margin:10px 0">
  <button onclick="openJoystickCalModal()" data-i18n="btn.joystickCal">手柄校准</button>
</div>
<div id="joystickCalStatus" style="font-size:12px;color:#8fa1b5;margin-bottom:8px">
  方向: -- / -- / -- | 油门: -- / -- / --
</div>
```

- [ ] **Step 2: 新增模态框 HTML**

在 `helpModal` 或其他 modal 之后插入：

```html
<div id="joystickCalModal" class="modal">
  <div class="dialog" style="max-width:480px">
    <h3 data-i18n="cal.title">手柄校准</h3>
    <div id="joystickCalStepText" style="margin:10px 0;line-height:1.5"></div>
    <div id="joystickCalLive" style="font-family:monospace;font-size:13px;color:#8fa1b5;margin:8px 0"></div>
    <div class="dialogActions">
      <button id="joystickCalActionBtn" onclick="joystickCalAction()">开始</button>
      <button id="joystickCalRetryBtn" onclick="joystickCalRetry()" style="display:none">重试</button>
      <button id="joystickCalSaveBtn" onclick="joystickCalSave()" style="display:none">保存</button>
      <button onclick="closeJoystickCalModal()">取消</button>
    </div>
  </div>
</div>
```

- [ ] **Step 3: 新增 i18n 条目**

在 `I18N` 字典中添加（按现有 `I18N.zh`/`I18N.en` 格式合并到对应对象内）：

```js
I18N.zh = {
  // ... existing keys ...
  'btn.joystickCal': '手柄校准',
  'cal.title': '手柄校准',
  'cal.step.center': '第 1 步：请将手柄（方向和油门）完全回中，保持不动，然后点击“开始”。',
  'cal.step.minmax': '第 2 步：请在 5 秒内将手柄依次推到最大位置：方向左、方向右、油门前、油门后。',
  'cal.step.done': '校准完成。请检查下方数值，确认后点击“保存”。',
  'cal.action.start': '开始校准',
  'cal.action.save': '保存到校車',
  'cal.action.retry': '重试',
};

I18N.en = {
  // ... existing keys ...
  'btn.joystickCal': 'Joystick Cal',
  'cal.title': 'Joystick Calibration',
  'cal.step.center': 'Step 1/2: Center both steering and throttle, then click Start.',
  'cal.step.minmax': 'Step 2/2: Within 5s move stick to all extremes: left, right, forward, back.',
  'cal.step.done': 'Calibration done. Review values below and click Save.',
  'cal.action.start': 'Start Calibration',
  'cal.action.save': 'Save to Car',
  'cal.action.retry': 'Retry',
};
```

- [ ] **Step 4: 新增 JS 逻辑**

在 `updateState(p)` 附近新增状态刷新：

```js
function refreshJoystickCalStatus(){
  fetch('/api/joystick-cal').then(r=>r.text()).then(t=>{
    const m=t.match(/steer_en=(\d+) st_min=(-?\d+) st_mid=(-?\d+) st_max=(-?\d+) throttle_en=(\d+) th_min=(-?\d+) th_mid=(-?\d+) th_max=(-?\d+)/);
    if(!m)return;
    const steerOn=m[1]==='1', thrOn=m[5]==='1';
    document.getElementById('joystickCalStatus').textContent=
      `方向: ${m[2]}/${m[3]}/${m[4]} [${steerOn?'ON':'OFF'}] | `+
      `油门: ${m[6]}/${m[7]}/${m[8]} [${thrOn?'ON':'OFF'}]`;
  });
}
```

新增模态控制（固件自动从 CENTERING 推进到 MINMAX 再 DONE，UI 只需轮询状态）：

```js
let calUiState='idle'; // idle | running | done
let calPollTimer=0;

function openJoystickCalModal(){
  document.getElementById('joystickCalModal').classList.add('show');
  calUiState='idle';
  renderCalStep();
}
async function closeJoystickCalModal(){
  if(calUiState==='running'){
    await fetch('/api/joystick-cal?action=abort',{method:'POST'});
  }
  document.getElementById('joystickCalModal').classList.remove('show');
  stopCalPoll();
}

function renderCalStep(firmwareState=0){
  const stepText=document.getElementById('joystickCalStepText');
  const live=document.getElementById('joystickCalLive');
  const actionBtn=document.getElementById('joystickCalActionBtn');
  const saveBtn=document.getElementById('joystickCalSaveBtn');
  const retryBtn=document.getElementById('joystickCalRetryBtn');
  saveBtn.style.display='none';
  retryBtn.style.display='none';
  if(calUiState==='idle'){
    stepText.textContent=t('cal.step.center');
    actionBtn.textContent=t('cal.action.start');
    actionBtn.style.display='inline-block';
    live.textContent='';
  }else if(calUiState==='running'){
    actionBtn.style.display='none';
    if(firmwareState===2){
      stepText.textContent=t('cal.step.minmax');
    }else{
      stepText.textContent=t('cal.step.center');
    }
  }else if(calUiState==='done'){
    stepText.textContent=t('cal.step.done');
    actionBtn.style.display='none';
    saveBtn.style.display='inline-block';
    retryBtn.style.display='inline-block';
  }
}

async function joystickCalAction(){
  const r=await fetch('/api/joystick-cal?action=start',{method:'POST'});
  const t=await r.text();
  if(t.startsWith('ACK')){
    calUiState='running';
    renderCalStep();
    startCalPoll();
  }else{
    showCommandError(t);
  }
}

async function joystickCalRetry(){
  const r=await fetch('/api/joystick-cal?action=retry',{method:'POST'});
  const t=await r.text();
  if(t.startsWith('ACK')){
    calUiState='running';
    renderCalStep();
    startCalPoll();
  }
}

async function joystickCalSave(){
  const r=await fetch('/api/joystick-cal?action=save',{method:'POST'});
  const t=await r.text();
  showCommandError(t);
  if(t.startsWith('ACK')){
    closeJoystickCalModal();
    refreshJoystickCalStatus();
  }
}

function startCalPoll(){
  stopCalPoll();
  calPollTimer=setInterval(async()=>{
    const r=await fetch('/api/joystick-cal');
    const t=await r.text();
    const m=t.match(/state=(\d+)/);
    if(!m)return;
    const state=parseInt(m[1]);
    const live=document.getElementById('joystickCalLive');
    live.textContent=t;
    if(state===1||state===2){
      renderCalStep(state);
    }else if(state===3){ // DONE
      calUiState='done';
      renderCalStep(state);
      stopCalPoll();
    }
  },500);
}
function stopCalPoll(){
  if(calPollTimer){clearInterval(calPollTimer);calPollTimer=0;}
}
```

在页面初始化或用户认证完成后调用一次 `refreshJoystickCalStatus()`，例如放在现有 `refreshStatus()` 末尾或 `applyLanguage()` 之后。

注意：`t('key')` 复用 `WebConsoleAssets.h` 中已有的翻译辅助函数（约 line 79）。

- [ ] **Step 5: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h
git commit -m "feat(joystick-cal): add calibration wizard to Drift Console"
```

---

## Task 14: 在主循环中调用校准更新

**Files:**
- Modify: `MUS4_FW/MUS4_FW.ino:417`

- [ ] **Step 1: 在 loop() 中调用 updateJoystickCalibration**

找到 `loop()` 中约 line 417：
```cpp
updateSteerCalibration();
```
替换为：

```cpp
updateJoystickCalibration();
```

- [ ] **Step 2: 编译检查**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 3: Commit**

```bash
git add MUS4_FW/MUS4_FW.ino
git commit -m "feat(joystick-cal): run calibration state machine in main loop"
```

---

## Task 15: 最终验证

**Files:**
- All of the above

- [ ] **Step 1: 运行 Python 测试**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
pytest tests/test_joystick_calibration.py tests/test_wireless_console_policy.py -v
```
Expected: All PASS

- [ ] **Step 2: 全量编译**

Run:
```bash
cd C:/Dev/DDC/Firmware/MUS4_FW
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
```
Expected: PASS

- [ ] **Step 3: 代码最终审查清单**

- [ ] `JoystickCalibration.cpp` 中 `mapJoystickAxis` 在 disabled 时使用 `RC_*` 默认值
- [ ] `JOYSTICK_SAVE` 保存前调用 `validateJoystickCalibration` 校验两轴
- [ ] `WirelessConsole.cpp` 与 `wireless_console_policy.py` 命令集合一致
- [ ] 旧 `SteeringCalibration.h/.cpp` 已删除，无残留引用
- [ ] `MUS4_FW.ino` 启动加载和主循环更新已替换
- [ ] Web Console 按钮在 `park == false` 时禁用（可选，固件侧已兜底）

- [ ] **Step 4: Commit any final fixes**

```bash
git commit -m "fix(joystick-cal): final review fixes" || true
```

---

## 验收标准

1. `pytest tests/test_joystick_calibration.py tests/test_wireless_console_policy.py` 全部通过。
2. `arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino` 编译通过。
3. Drift Console 出现“手柄校准”按钮，点击后弹出两步向导。
4. 校准完成后刷新页面，状态显示区展示新的 min/mid/max。
5. 切换 MANUAL / SEMI_AUTO / FULL_AUTO 模式时，手动 RC 输出与模型输出在 `[-100, 100]` 上对齐。
6. 旧设备已有方向盘校准数据时，首次启动自动迁移到新 key，不丢失。

---

## 依赖顺序图

```text
Task 1 (NVS keys)
    ↓
Task 2 (Header)
    ↓
Task 3 (Implementation)
    ↓
Task 4 (Boot load) ──→ Task 14 (Main loop update)
    ↓
Task 5 (ControlMixer)
    ↓
Task 6 (Commands)
    ↓
Task 7 (Wireless permissions) ──→ Task 8 (Python policy)
    ↓
Task 9 (Delete old files)
    ↓
Task 10 (Unit tests) ──→ Task 11 (Policy tests)
    ↓
Task 12 (Web API) ──→ Task 13 (Web UI)
    ↓
Task 15 (Final verification)
```
