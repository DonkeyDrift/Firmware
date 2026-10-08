#pragma once

#include <Arduino.h>
#include "FirmwareConfig.h"

// ── Phase 1: FirmwareApp 任务调度表 ──────────────────────────────────────
// 将 loop() 中散落的 if (millis() - lastX >= intervalX) 分支抽取为统一的
// Task 调度表，时序策略与业务代码解耦。
//
// 设计原则（见 docs/Plan/MUS4_FW模块化拆分方案.md §4）：
// - 不引入复杂 RTOS，只把时序策略从业务代码中剥离
// - 每个 Task 有独立的 intervalMs / lastRunMs，互不干扰
// - enabled 标志支持运行时启停（如 OTA 期间暂停遥测）
// - callback 无参无返回，业务逻辑在各模块内实现

struct FirmwareTask {
    const char* name;
    uint32_t intervalMs;      // 0 = 每次 loop() 都执行
    uint32_t lastRunMs;
    void (*callback)();
    bool enabled;
};

// 最大任务数（增删任务时同步调整）
#define FIRMWARE_TASK_COUNT 12

// 任务 ID 枚举（与 firmware_tasks[] 数组下标对应）
enum FirmwareTaskId {
    TASK_SENSOR_READ = 0,     // 读 INA219 + MPU6050
    TASK_JOYSTICK_CAL,        // 摇杆校准
    TASK_SERIAL_READ,         // 读 Serial0/1/2
    TASK_WIFI_CONSOLE,        // Wi-Fi 控制台
    TASK_WIFI_WEB,            // Wi-Fi Web 控制台
    TASK_WIFI_STA,            // Wi-Fi STA 状态机
    TASK_CLOUD_REPORT,        // 云端上报
    TASK_WIFI_STA_HISTORY,    // STA 历史重试
    TASK_WIFI_BOOT_RESET,     // 开机重置按钮
    TASK_WIFI_OTA,            // OTA 更新
    TASK_RC_FILTER,           // RC 滤波
    TASK_TELEMETRY,           // 遥测发送（T/S + M:P + $IMU）
};

class FirmwareApp {
public:
    FirmwareApp();
    void begin();
    void update();
    void setTaskEnabled(FirmwareTaskId id, bool enabled);
    bool isTaskEnabled(FirmwareTaskId id) const;

private:
    FirmwareTask tasks[FIRMWARE_TASK_COUNT];
    void runTask(FirmwareTask& task, uint32_t nowMs);
};

// 全局实例（单例模式，与现有全局变量风格一致）
extern FirmwareApp firmwareApp;
