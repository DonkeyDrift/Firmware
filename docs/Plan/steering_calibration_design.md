# Steering 校准功能设计方案

## 1. 需求分析

### 1.1 问题现状
- CH1（Steering）手柄中位对应的物理前轮方向并非正前，当前实际 `STEERING=-10` 才是对正方向。
- 转向机构存在机械行程限制，左右满舵无法达到理论极限 ±100。
- 当前映射使用硬编码常量 `RC_STEERING_MIN=872`, `RC_STEERING_MID=1488`, `RC_STEERING_MAX=2113`，无运行时校准能力。

### 1.2 目标
在固件中增加转向校准功能，实现：
1. 通过交互式向导记录实际中位、最左、最右 PWM 值。
2. 根据记录值计算校准系数，修正 `map()` 映射逻辑。
3. 在 TUI 界面上实时显示校准步骤、关键数值和波形辅助观察。
4. 将校准参数持久化到 NVS，重启后自动生效。

---

## 2. 方案选项

### 选项 A：TUI 串口校准向导（推荐）
利用现有 ANSI 终端 TUI，增加**校准模式状态机**。用户通过串口命令触发和推进校准流程，TUI 切换为校准向导界面，实时显示操作提示、当前 PWM 采样值、统计稳定性及已记录结果。

**优点**：
- 完全基于现有架构（TUI + Serial + Preferences），无需新增网络/前端依赖。
- 实现紧凑，代码侵入性小，与固件安全关键逻辑耦合低。
- 串口命令可被 USB Serial、Serial1、TCP Console 统一访问，兼容现有无线控制台。

**缺点**：
- 无真正图形按钮，交互依赖文本命令（`CALIBRATE_STEER`、`NEXT`、`ABORT`）。
- TUI 波形图为 ASCII 字符，观察精度有限。

### 选项 B：Wi-Fi Web Console 校准页面
在现有 Web Server 中新增 `/calibrate.html` 页面，使用 HTML5 Canvas 绘制 Steering 实时曲线，提供可视化按钮和步骤引导，通过 HTTP/WebSocket 与固件交互完成采样和保存。

**优点**：
- 真正的图形曲线和按钮，用户体验更好。
- 可以绘制更精确的实时曲线和校准前后对比图。

**缺点**：
- 工作量大：需开发前端页面、HTTP API、状态同步逻辑。
- 依赖 Wi-Fi Console 编译开关，未启用 Wi-Fi 时无法使用。
- 与当前 TUI 波形图语境关联较弱。

> **建议**：优先采用选项 A。若后续需要更丰富的图形化校准界面，可在选项 A 的 NVS 数据模型和底层映射逻辑基础上，再叠加选项 B 的 Web UI。

---

## 3. 详细设计（选项 A）

### 3.1 数据模型与持久化

#### 3.1.1 新增 NVS 键值
在 `mus4.ino` 的 Preferences 命名空间 `"mus4"` 下新增 3 个 `int` 型键：

| 键名 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| `steer_c` | `int` | Steering 中位（0 位）PWM | 1488 |
| `steer_l` | `int` | Steering 最左（最小）PWM | 872 |
| `steer_r` | `int` | Steering 最右（最大）PWM | 2113 |

#### 3.1.2 运行时代理变量
```cpp
// 从 NVS 加载后的实际校准值，未校准时回退到编译期常量
int steerCenterPWM = RC_STEERING_MID;
int steerLeftPWM   = RC_STEERING_MIN;
int steerRightPWM  = RC_STEERING_MAX;
bool steerCalibrated = false;  // NVS 中是否存在有效校准值
```

#### 3.1.3 读写函数
- `loadSteeringCalibration()` — `setup()` 中调用，从 NVS 读取 `steer_c/l/r`。
- `saveSteeringCalibration(int c, int l, int r)` — 校准完成后写入 NVS。
- `clearSteeringCalibration()` — 恢复默认并清除 NVS 键。
- `hasSteeringCalibration()` — 判断 NVS 中是否存在有效校准值。

**有效性校验**：
- `left < center < right`
- `right - left >= 200`（最小有效行程，防止误记录）
- 所有值在 `RC_PWM_MIN(800)` ~ `RC_PWM_MAX(2200)` 范围内

### 3.2 TUI 校准界面

#### 3.2.1 校准状态机（ mus4.ino 层）
```cpp
enum SteerCalState {
    STEER_CAL_IDLE,      // 未在校准
    STEER_CAL_CENTER,    // 步骤1：记录中位
    STEER_CAL_LEFT,      // 步骤2：记录最左
    STEER_CAL_RIGHT,     // 步骤3：记录最右
    STEER_CAL_REVIEW     // 步骤4：预览并确认保存
};
```

#### 3.2.2 TUI 新增接口（TUI.h / TUI.cpp）
```cpp
// TUI.h
enum class TUICalStep { NONE, CENTER, LEFT, RIGHT, REVIEW };

void setCalibrationMode(bool enabled, TUICalStep step = TUICalStep::NONE);
void setCalibrationData(int currentPWM, int avgPWM, int stdPWM,
                        int recordedCenter, int recordedLeft, int recordedRight);
void setCalibrationResult(float scaleLeft, float scaleRight, int outMin, int outMax);
```

#### 3.2.3 校准界面布局（替代正常仪表盘）
当 `_calibrationMode == true` 时，`render()` 跳过 `drawMode/drawPark/drawOutput/drawWaveforms/drawSensors`，改为调用 `drawCalibration()`。

```text
Row 1  [Header]    DonkeyCar Control System - v1.5.4
Row 2  [分隔线]    ===================================
Row 3  [Title]     === Steering Calibration ===
Row 4  [Step]      Step 1/3: Record CENTER (straight ahead)
Row 5  [Hint]      Keep steering wheel centered. Do not move.
Row 6  [Live]      Live PWM: 1488  Avg: 1487  Std: 2
Row 7  [Status]    Samples: 42/50  [STABLE]
Row 8  [Recorded]  Center: --  Left: --  Right: --
Row 9  [Command]   Send NEXT when ready, or ABORT to cancel
Row 11 [Wave]      Steering Live Waveform: (启用实时波形辅助观察)
```

- **实时波形**：在校准模式下临时启用 Steering 波形图（即使正常模式下被禁用），帮助用户观察信号稳定性。
- **稳定性指示**：标准差 `stdPWM < 5` 时显示绿色 `[STABLE]`，否则黄色 `[UNSTABLE]`。

#### 3.2.4 脏矩形策略
校准界面同样遵循脏矩形增量刷新：仅当 `_calibrationMode`、步骤、数值发生变化时重绘对应行。

### 3.3 校准算法

#### 3.3.1 采样逻辑（主循环 8ms RC 更新节拍内）
```cpp
if (steerCalState != STEER_CAL_IDLE) {
    static uint16_t calSamples[50];
    static uint8_t calSampleIndex = 0;
    
    // 每 8ms 采集一个 pwm_filtered[CH_STEERING] 样本
    calSamples[calSampleIndex++] = pwm_filtered[CH_STEERING];
    if (calSampleIndex >= 50) {
        // 计算平均值与标准差
        int avg = calculateAverage(calSamples, 50);
        int std = calculateStdDev(calSamples, 50);
        
        // 自动推进或等待用户确认（根据设计选择）
        // 推荐：自动检测稳定后提示用户发送 NEXT
        tui.setCalibrationData(livePWM, avg, std, ...);
        calSampleIndex = 0; // 滚动更新，保持实时统计
    }
}
```

**两种交互模式供决策**：
- **模式 A（自动检测）**：当连续 50 个样本标准差 < 5 且维持 1 秒，自动锁定该步骤数值，提示用户发送 `NEXT` 进入下一步。
- **模式 B（完全手动）**：用户观察实时数据，自行判断稳定后发送 `NEXT`，系统立即记录当前平均值。

> **推荐模式 B**：更简单可靠，避免自动检测误判。用户发送 `NEXT` 时取最近 50 点平均。

#### 3.3.2 校准系数计算（步骤 4 REVIEW）
记录三个原始 PWM 值后，计算并显示：

```cpp
// 映射系数（用于替换现有 map 逻辑）
int mapSteering(int rawPWM) {
    if (rawPWM <= steerCenterPWM) {
        return map(rawPWM, steerLeftPWM, steerCenterPWM, -100, 0);
    } else {
        return map(rawPWM, steerCenterPWM, steerRightPWM, 0, 100);
    }
}

// 显示信息
leftRange  = steerCenterPWM - steerLeftPWM;   // 例如 616 µs
rightRange = steerRightPWM - steerCenterPWM;  // 例如 625 µs
leftScale  = 100.0f / leftRange;              // µs/单位
rightScale = 100.0f / rightRange;
```

TUI REVIEW 界面显示：
```text
Row 4  [Result]    Calibration Complete
Row 5  [Values]    Center: 1493  Left: 880  Right: 2100
Row 6  [Range]     LeftRange: 613us  RightRange: 607us
Row 7  [Scale]     LeftScale: 0.163  RightScale: 0.165
Row 8  [Output]    Output limits: -100 to +100
Row 9  [Command]   SEND SAVE to persist, or RETRY to restart
```

### 3.4 映射公式修改

#### 3.4.1 当前代码（mus4.ino ~3634）
```cpp
car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
```

#### 3.4.2 新映射函数
```cpp
static int applySteeringCalibration(int rawPWM) {
    if (!steerCalibrated) {
        // 无校准时回退到原硬编码逻辑
        return map(rawPWM, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
    }
    
    // 限幅保护
    rawPWM = constrain(rawPWM, steerLeftPWM, steerRightPWM);
    
    if (rawPWM <= steerCenterPWM) {
        return map(rawPWM, steerLeftPWM, steerCenterPWM, -100, 0);
    } else {
        return map(rawPWM, steerCenterPWM, steerRightPWM, 0, 100);
    }
}
```

替换主循环调用：
```cpp
car_output.steering = applySteeringCalibration(rc_data.steering);
```

**安全考量**：
- `constrain()` 确保即使校准值有误，也不会产生超出范围的输出。
- 保留原有 `RC_STEERING_MIN/MAX` 作为全局硬边界（`acceptRcPulse` 层不变）。
- 若 NVS 加载的校准值未通过有效性校验，自动丢弃并回退到编译期默认值。

### 3.5 串口命令扩展

在 `mus4.ino` 的 `processLine()` 和 `PROCESS_COMMAND_LINE` 宏中增加：

| 命令 | 作用 | 权限要求 |
|------|------|---------|
| `STEER_CAL` | 进入转向校准模式（从 IDLE → CENTER）| 无（本地串口）/ 需认证（无线） |
| `CAL_NEXT` | 确认当前步骤，进入下一步 | 校准进行中 |
| `CAL_ABORT` | 取消校准，返回正常模式 | 校准进行中 |
| `CAL_SAVE` | 在 REVIEW 步骤保存校准值到 NVS | 校准 REVIEW 阶段 |
| `CAL_RESET` | 清除 NVS 校准值，恢复默认 | 无（本地串口） |

无线控制台权限策略（`wireless_console_policy.py`）需同步更新，将 `STEER_CAL` / `CAL_NEXT` / `CAL_ABORT` / `CAL_SAVE` / `CAL_RESET` 纳入权限矩阵。

### 3.6 TUI 正常模式提示
在正常仪表盘底部（`drawSensors` 最后一行），若当前存在有效的 Steering 校准值，显示一行提示：
```text
[Steering CAL active: C=1493 L=880 R=2100]
```
若无校准值，显示：
```text
[Send STEER_CAL to calibrate steering]
```

---

## 4. 文件修改清单

| 文件 | 修改内容 |
|------|---------|
| `mus4.ino` | 1. 新增 NVS 键名常量、运行时代理变量、校准状态机枚举。<br>2. 新增 `load/save/clear/has SteeringCalibration()` 函数。<br>3. 新增 `applySteeringCalibration()` 映射函数。<br>4. 修改主循环 steering map 调用。<br>5. 新增校准采样逻辑（8ms RC 更新节拍内）。<br>6. 扩展 `processLine()` 和 `PROCESS_COMMAND_LINE` 宏支持校准命令。<br>7. `setup()` 中调用 `loadSteeringCalibration()`。 |
| `TUI.h` | 1. 新增 `TUICalStep` 枚举。<br>2. 新增 `setCalibrationMode()`、`setCalibrationData()`、`setCalibrationResult()` 公共接口。<br>3. 新增私有状态字段：`_calibrationMode`、`_calStep`、`_calData` 等。<br>4. 新增私有 `drawCalibration()` 声明。 |
| `TUI.cpp` | 1. 实现 `setCalibrationMode()`、`setCalibrationData()`、`setCalibrationResult()`。<br>2. 修改 `render()`：校准模式下调用 `drawCalibration()` 替代正常绘制。<br>3. 实现 `drawCalibration()`：步骤标题、提示文本、实时数值、稳定性、已记录值、波形图、命令提示。<br>4. 修改 `drawSensors()`：底部增加校准状态提示。 |
| `wireless_console_policy.py` | 新增 `STEER_CAL`, `CAL_NEXT`, `CAL_ABORT`, `CAL_SAVE`, `CAL_RESET` 命令的权限定义（本地串口公开，无线需认证）。 |
| `tests/test_wireless_console_policy.py` | 新增上述命令的权限单元测试。 |
| `Doc/Plan/steering_calibration_design.md` | 保存本设计文档（正式版本）。 |

---

## 5. 时序与性能影响

- **校准采样**：复用现有的 8ms RC 滤波更新节拍，不新增定时器。每步 50 样本约 400ms，总流程约 2-5 秒（含用户操作时间）。
- **TUI 刷新**：校准界面绘制复杂度与正常仪表盘相当，不额外增加渲染负担。
- **主循环**：`applySteeringCalibration()` 为简单整数运算（1 次比较 + 1 次 `map`），对 200Hz 主循环零可感知影响。
- **NVS 写入**：仅在校准保存时触发 1 次 `Preferences::putInt` × 3，耗时 < 10ms，非实时路径。

---

## 6. 风险与注意事项

1. **安全关键**：Steering 映射直接影响舵机输出。`applySteeringCalibration()` 必须包含 `constrain()` 和有效性校验。校准期间应建议用户将车辆抬离地面（前轮悬空）。
2. **向后兼容**：未校准时必须 100% 回退到原有硬编码 `map()` 行为，避免影响未校准用户。
3. **NVS 空间**：ESP32 NVS 默认分区足够，但需确认键名长度不超限（当前键名 7 字符，安全）。
4. **多客户端并发**：若通过 TCP Console 触发校准，需确保仅一个客户端能进入校准模式（增加 `calibrationClientId` 锁）。
5. **Park 状态建议**：校准向导界面应提示用户在 Park LOCKED 状态下进行（前轮无动力，更安全）。
