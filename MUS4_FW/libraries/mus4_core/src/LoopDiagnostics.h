#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

// 主 loopTask 看门狗 + 帧耗时诊断（v1.10.17 loop-hang fix）。
//
// Arduino core 的 app_main() 写死 loopTaskWDTEnabled=false，loopTask 从不订阅
// Task WDT——主 loop 卡死只会静默停摆（RC/控制/遥测全部冻结）而不复位。
// 本模块补上两层：
//   1) esp_task_wdt 订阅与每帧喂狗（LOOP_TASK_WDT_ENABLED / LOOP_TASK_WDT_TIMEOUT_S）；
//   2) 主 loop 帧间隔统计（last/max/frames），供 STATUS 与 /api/status 输出
//      loop_dt_max_ms——复盘「动手柄无反应」类故障时判断 loop 是否停摆。
//
// 订阅时机约定：loopDiagnosticsBegin() 必须在 setup() 末尾调用——Wi-Fi/STA 首连
// 在 setup 内可能合法阻塞 >5s（WIFI_STA_CONNECT_TIMEOUT_MS=15000），提前订阅会
// 把正常开机误判成死机。loopDiagnosticsFeed() 在 loop() 末尾每帧调用。
// 读取侧（任意任务，含 Web 任务）经 loopDiagnosticsSnapshot()，内部临界区。

struct LoopDiagnosticsSnapshot {
    uint32_t lastDtMs = 0;  // 最近一帧 loop 间隔（ms）
    uint32_t maxDtMs = 0;   // 本次开机 loop 间隔峰值（ms），即 STATUS 的 loop_dt_max_ms
    uint32_t frames = 0;    // 本次开机 loop 帧计数
};

// 订阅 loopTask 到 Task WDT 并初始化帧统计。重复调用安全（只生效一次）。
void loopDiagnosticsBegin();

// loop() 末尾每帧调用：喂狗 + 记录帧间隔。
void loopDiagnosticsFeed();

// 任意任务读取当前帧统计快照（内部临界区，可从 Web 任务调用）。
void loopDiagnosticsSnapshot(LoopDiagnosticsSnapshot& out);

// esp_reset_reason() → 可读文本（如 "TASK_WDT"/"PANIC"/"BROWNOUT"）。
// 未知枚举回退 "OTHER_<数值>"，不会输出空串。
void espResetReasonText(char* buf, size_t len);
