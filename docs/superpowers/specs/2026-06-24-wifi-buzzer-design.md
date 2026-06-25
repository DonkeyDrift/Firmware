# Wi-Fi 状态蜂鸣器提示音设计

## 背景与目标

调试 MUS4 设备的 Wi-Fi 行为时，经常需要判断当前处于 AP 模式还是 STA 模式、STA 是否已上联、AP 是否已恢复。目前只能依赖串口日志或观察路由器/手机连接状态，现场调试不便。

本设计为以下四种 Wi-Fi 状态转换增加短促、可区分的蜂鸣器提示音：

1. AP 启动（开机、STA 失败后恢复 AP、手动触发 AP 重启）
2. AP 关闭（STA 稳定后进入 STA-only）
3. STA 连接成功（首次获取到 `WL_CONNECTED`）
4. STA 断开/失败（down grace 超时、no_ssid、auth_failed、timeout 后切回 AP）

## 音效映射

| 事件 | 旋律 | 设计意图 |
|---|---|---|
| AP 启动 | C4-E4-G4（上升，八分音符×2 + 四分音符） | 与解锁音同风格，表示“进入可连接状态” |
| AP 关闭 | G4-E4-C4（下降，八分音符×2 + 四分音符） | 与锁定音同风格，表示“退出可连接状态” |
| STA 连接成功 | G4-G4（两个短八分音符） | 轻快双响，表示“上联成功” |
| STA 断开/失败 | C4（一个长四分音符） | 单长低音，表示“失去上联” |

## 架构决策

### 方案选择：直接调用全局 `buzzer`

在 `WifiManager.cpp` 顶部增加 `extern Buzzer buzzer;`，在状态转换点直接调用新增加的蜂鸣器接口。

**理由：**
- 与现有 `ControlMixer.cpp`、`SafetyState.cpp` 的调用方式一致，符合项目惯例。
- 改动最小，不需要新增回调注册或事件分发机制。
- 蜂鸣器播放是非阻塞的，`buzzer.update()` 已在主循环中调用，不影响 Wi-Fi 状态机时序。

**权衡：**
- `mus4_wifi` 库会直接依赖 `mus4_ui` 的 `Buzzer` 类。但现有代码已存在同类依赖，因此可接受。

## 实现改动

### 1. `libraries/mus4_ui/src/Buzzer.h`

新增 4 个 public 方法声明：

```cpp
void playWifiApStartSound();
void playWifiApStopSound();
void playWifiStaConnectedSound();
void playWifiStaDisconnectedSound();
```

### 2. `libraries/mus4_ui/src/Buzzer.cpp`

新增 4 个 `BuzzerNote` 旋律数组及对应实现：

```cpp
BuzzerNote melodyWifiApStart[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

BuzzerNote melodyWifiApStop[] = {
    { NOTE_G4, N8 },
    { NOTE_E4, N8 },
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

BuzzerNote melodyWifiStaConnected[] = {
    { NOTE_G4, N8 },
    { NOTE_G4, N8 },
    { NOTE_REST, N8 }
};

BuzzerNote melodyWifiStaDisconnected[] = {
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};
```

并新增对应的方法实现，统一使用 `startMelody()`。

### 3. `libraries/mus4_wifi/src/WifiManager.cpp`

- 顶部增加 `extern Buzzer buzzer;`
- `startWifiApServices()` 成功启动 AP 和 console 服务后调用 `buzzer.playWifiApStartSound()`
- `stopWifiApForStaOnly()` 末尾调用 `buzzer.playWifiApStopSound()`
- `updateWifiSta()` 首次检测到 `WL_CONNECTED` 时调用 `buzzer.playWifiStaConnectedSound()`
- `restoreApAfterStaLost()` 开头调用 `buzzer.playWifiStaDisconnectedSound()`，确保所有 STA 失败/断开路径统一发声

## 测试计划

1. 编译验证：确保 `Buzzer.cpp` 和 `WifiManager.cpp` 无编译错误。
2. 静态测试：运行 `pytest tests/test_firmware_feature_flags.py`，确认现有特征检测未被破坏。
3. 实机验证：上传固件后依次验证：
   - 开机 AP 启动是否有上升提示音
   - 保存正确 STA 配置后，STA 连接成功是否有双短提示音
   - STA 稳定后 AP 关闭是否有下降提示音
   - 断开过久或保存错误 STA 配置导致失败后，恢复 AP 是否有单长低音

## 边界与注意事项

- 蜂鸣器播放是非阻塞的，不会影响 Wi-Fi 状态机。
- 声音播放与现有日志并行，不替代日志。
- 音量沿用现有 `BUZZER_VOLUME`（40），不做单独调整。
- 如果未来希望关闭 Wi-Fi 提示音，可在 `Buzzer` 中增加按场景静音的能力，但不在本设计范围内。
