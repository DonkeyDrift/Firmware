# Wi-Fi 状态蜂鸣器提示音实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 AP 启动、AP 关闭、STA 连接成功、STA 断开/失败四种 Wi-Fi 状态转换增加可区分的蜂鸣器提示音。

**Architecture:** 在 `mus4_ui` 的 `Buzzer` 类中新增 4 条旋律和 4 个播放方法；在 `mus4_wifi` 的 `WifiManager.cpp` 中通过全局 `extern Buzzer buzzer` 在状态转换点调用对应方法。保持蜂鸣器非阻塞特性，不影响 Wi-Fi 状态机。

**Tech Stack:** C++ / Arduino-ESP32 / ESP32 `ledc` PWM / 项目自有的 `mus4_ui` 与 `mus4_wifi` 库

---

## 文件结构

| 文件 |  responsibility  |
|---|---|
| `MUS4_FW/libraries/mus4_ui/src/Buzzer.h` | 声明 4 个新的 public 播放方法 |
| `MUS4_FW/libraries/mus4_ui/src/Buzzer.cpp` | 实现 4 条旋律数组与 4 个播放方法 |
| `MUS4_FW/libraries/mus4_wifi/src/WifiManager.cpp` | 在 4 个 Wi-Fi 状态转换点调用新的蜂鸣器方法 |

---

### Task 1: 扩展 Buzzer 类，新增 Wi-Fi 事件旋律

**Files:**
- Modify: `MUS4_FW/libraries/mus4_ui/src/Buzzer.h`
- Modify: `MUS4_FW/libraries/mus4_ui/src/Buzzer.cpp`

- [ ] **Step 1: 在头文件中声明 4 个新方法**

在 `Buzzer.h` 的 public 区域，紧接现有方法之后添加：

```cpp
void playWifiApStartSound();
void playWifiApStopSound();
void playWifiStaConnectedSound();
void playWifiStaDisconnectedSound();
```

- [ ] **Step 2: 在实现文件中添加旋律数组与方法**

在 `Buzzer.cpp` 中，现有旋律数组之后、`Buzzer::Buzzer` 构造函数之前，添加：

```cpp
// Wi-Fi AP 启动提示音 - 上升音阶 C4-E4-G4
BuzzerNote melodyWifiApStart[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

// Wi-Fi AP 关闭提示音 - 下降音阶 G4-E4-C4
BuzzerNote melodyWifiApStop[] = {
    { NOTE_G4, N8 },
    { NOTE_E4, N8 },
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

// Wi-Fi STA 连接成功提示音 - 双短高音 G4-G4
BuzzerNote melodyWifiStaConnected[] = {
    { NOTE_G4, N8 },
    { NOTE_G4, N8 },
    { NOTE_REST, N8 }
};

// Wi-Fi STA 断开/失败提示音 - 单长低音 C4
BuzzerNote melodyWifiStaDisconnected[] = {
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};
```

在 `Buzzer.cpp` 末尾，现有 `update()` 方法之后，添加：

```cpp
void Buzzer::playWifiApStartSound() {
    startMelody(melodyWifiApStart, sizeof(melodyWifiApStart) / sizeof(BuzzerNote));
}

void Buzzer::playWifiApStopSound() {
    startMelody(melodyWifiApStop, sizeof(melodyWifiApStop) / sizeof(BuzzerNote));
}

void Buzzer::playWifiStaConnectedSound() {
    startMelody(melodyWifiStaConnected, sizeof(melodyWifiStaConnected) / sizeof(BuzzerNote));
}

void Buzzer::playWifiStaDisconnectedSound() {
    startMelody(melodyWifiStaDisconnected, sizeof(melodyWifiStaDisconnected) / sizeof(BuzzerNote));
}
```

- [ ] **Step 3: 编译验证 Buzzer 改动**

运行：

```bash
cd MUS4_FW && python arduino-cli.py -c --sketch MUS4_FW.ino
```

Expected: 编译通过（此时 `WifiManager.cpp` 尚未调用新方法，因此只验证 Buzzer 本身无语法错误）。

---

### Task 2: 在 Wi-Fi 状态机中接入蜂鸣器提示音

**Files:**
- Modify: `MUS4_FW/libraries/mus4_wifi/src/WifiManager.cpp`

- [ ] **Step 1: 引入全局 Buzzer 实例**

在 `WifiManager.cpp` 顶部、其他 `extern` 声明附近，添加：

```cpp
extern Buzzer buzzer;
```

- [ ] **Step 2: AP 启动成功后播放提示音**

在 `startWifiApServices()` 中，`startWifiConsoleServices(logPrefix)` 调用成功后、函数返回前，添加：

```cpp
buzzer.playWifiApStartSound();
```

目标代码上下文：

```cpp
    return startWifiConsoleServices(logPrefix);
}
```

替换为：

```cpp
    bool ok = startWifiConsoleServices(logPrefix);
    if (ok) {
        buzzer.playWifiApStartSound();
    }
    return ok;
}
```

- [ ] **Step 3: AP 关闭后播放提示音**

在 `stopWifiApForStaOnly()` 末尾、`mus4LogLine("wifi", "AP stopped after STA connected");` 之后，添加：

```cpp
    buzzer.playWifiApStopSound();
```

- [ ] **Step 4: STA 首次连接成功后播放提示音**

在 `updateWifiSta()` 中，首次进入 `WL_CONNECTED` 分支、设置 `wifiStaConnected = true` 后，添加：

```cpp
            buzzer.playWifiStaConnectedSound();
```

目标上下文（在 `clearWifiStaLastError();` 之后、`startWifiMdnsIfNeeded();` 之前）：

```cpp
            wifiStaConnected = true;
            wifiStaTimedOut = false;
            wifiStaConnecting = false;
            clearWifiStaLastError();
            buzzer.playWifiStaConnectedSound();
            startWifiMdnsIfNeeded();
```

- [ ] **Step 5: STA 断开/失败时统一播放提示音**

在 `restoreApAfterStaLost()` 开头，重置 grace 等状态之后，添加：

```cpp
    buzzer.playWifiStaDisconnectedSound();
```

目标上下文（在 `wifiStaApplyFromAp = false;` 之后、`wifiStaConnected = false;` 之前）：

```cpp
static void restoreApAfterStaLost()
{
    wifiStaUpGraceDeadlineMs = 0;
    wifiStaDownGraceDeadlineMs = 0;
    wifiStaApplyFromAp = false;
    buzzer.playWifiStaDisconnectedSound();
    wifiStaConnected = false;
    wifiStaConnecting = false;
```

- [ ] **Step 6: 编译验证完整改动**

运行：

```bash
cd MUS4_FW && python arduino-cli.py -c --sketch MUS4_FW.ino
```

Expected: 编译通过。若报错 `Buzzer` 未声明，检查 `WifiManager.cpp` 中是否已添加 `#include "Buzzer.h"`（可通过 `mus4_ui.h` 间接包含，但建议显式包含 `<Buzzer.h>` 或 `<mus4_ui.h>`）。

---

### Task 3: 运行静态特征测试

**Files:**
- Test: `MUS4_FW/tests/test_firmware_feature_flags.py`

- [ ] **Step 1: 运行 feature flags 测试**

运行：

```bash
cd MUS4_FW && python -m pytest tests/test_firmware_feature_flags.py
```

Expected: 全部通过（当前 97 passed）。本改动不修改编译开关、Web Console 前端或状态机核心字段，因此不应破坏任何现有断言。

- [ ] **Step 2: 若测试失败则修复**

如果某个断言因新增代码而失败，根据错误信息判断：
- 若断言检查 `Buzzer.cpp` 中特定旋律数量或模式音符号，需要同步更新测试；
- 若断言检查 `WifiManager.cpp` 中某些函数调用或字段，检查是否意外修改了相关行。

---

### Task 4: 固件上传与实机验证

**Files:**
- Build output: `MUS4_FW/build/MUS4_FW.ino.bin`

- [ ] **Step 1: 编译并上传到目标设备**

根据当前环境选择上传方式。若使用串口：

```bash
cd MUS4_FW && python arduino-cli.py -cu --sketch MUS4_FW.ino --port COM20
```

若当前目标通过 HTTP OTA：

```bash
cd MUS4_FW && .\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -Sketch MUS4_FW.ino
```

Expected: 上传成功，设备复位后启动。

- [ ] **Step 2: 验证 AP 启动提示音**

设备复位或开机，监听蜂鸣器。

Expected: 听到上升三音 C4-E4-G4，随后 AP `MU04TE-ESP`（或当前配置的 AP SSID）出现。

- [ ] **Step 3: 验证 STA 连接成功提示音**

通过 Web Console 或串口配置正确的 STA 凭据并应用：

```text
WIFI_STA_SSID your_ssid
WIFI_STA_PASSWORD your_pass
WIFI_STA_APPLY
```

Expected: 约 1 秒后听到轻快双响 G4-G4；日志出现 `STA connected IP: ...`。

- [ ] **Step 4: 验证 AP 关闭提示音**

等待 STA 稳定（`WIFI_STA_GRACE_UP_MS` 约 1000ms 后）。

Expected: 听到下降三音 G4-E4-C4；日志出现 `AP stopped after STA connected`；手机/电脑不再看到 AP SSID。

- [ ] **Step 5: 验证 STA 断开/失败提示音**

两种验证方式：

a) 在 STA-only 状态下关闭路由器或让设备远离信号，等待 `WIFI_STA_GRACE_DOWN_MS` 超时。

b) 通过串口保存一个错误密码或不存在 SSID：

```text
WIFI_STA_SSID fakenet
WIFI_STA_PASSWORD wrongpass
WIFI_STA_APPLY
```

Expected: 听到单长低音 C4；日志出现 `AP restored after STA ...`；AP 重新出现。

---

## 自审

**1. Spec coverage:**
- 四种事件音效映射 → Task 1 Step 2 已覆盖。
- AP 启动/关闭触发点 → Task 2 Step 2 / Step 3 已覆盖。
- STA 连接成功触发点 → Task 2 Step 4 已覆盖。
- STA 断开/失败触发点 → Task 2 Step 5 已覆盖（`restoreApAfterStaLost` 统一处理 down grace/no_ssid/auth_failed/timeout 四条路径）。
- 编译与测试 → Task 3 已覆盖。
- 实机验证 → Task 4 已覆盖。

**2. Placeholder scan:**
- 无 "TBD"、"TODO"、"implement later"。
- 所有代码块包含实际音符、方法名和调用位置。
- 所有命令包含实际路径和预期结果。

**3. Type consistency：**
- `BuzzerNote`、 `startMelody()`、`buzzer.play...Sound()` 命名与现有 `playParkLockSound()` 等保持一致。
- `extern Buzzer buzzer;` 与 `MUS4_FW.ino` 中全局实例类型一致。
