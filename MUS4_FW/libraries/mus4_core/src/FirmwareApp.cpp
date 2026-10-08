#include "FirmwareApp.h"

// ── Phase 1: FirmwareApp 任务调度实现 ──────────────────────────────────────

// 前向声明：各任务的回调函数（定义在 MUS4_FW.ino 或各模块中）
// 注意：这些函数必须在 FirmwareApp.update() 调用前完成初始化。
void read_ina219();
void read_mpu6050();
void updateJoystickCalibration();
void readSerialBuf(Stream& s, SerialBuf& buf);
void handleSerial2();
#ifdef ENABLE_WIFI_CONSOLE
void updateWifiConsole();
void updateWifiWebConsole();
void updateWifiSta();
void updateWifiStaHistoryRetry();
void updateWifiBootResetButton();
void updateWifiOta(OtaRuntimeState& os, WifiRuntimeState& ws);
namespace mus4cloud { void update(); }
#endif
void updateRcFilter();
void emitTelemetry();

// 传感器读取任务（100Hz）
static void taskSensorRead() {
    read_ina219();
    read_mpu6050();
}

// RC 滤波任务（RC_FILTER_UPDATE_INTERVAL Hz）
static void taskRcFilter() {
    updateRcFilter();
}

// 遥测发送任务（60Hz + 100Hz IMU + 1Hz M:P）
static void taskTelemetry() {
    emitTelemetry();
}

// Wi-Fi 相关任务（ENABLE_WIFI_CONSOLE）
#ifdef ENABLE_WIFI_CONSOLE
static void taskWifiConsole() { updateWifiConsole(); }
static void taskWifiWeb() { updateWifiWebConsole(); }
static void taskWifiSta() { updateWifiSta(); }
static void taskCloudReport() { mus4cloud::update(); }
static void taskWifiStaHistory() { updateWifiStaHistoryRetry(); }
static void taskWifiBootReset() { updateWifiBootResetButton(); }
static void taskWifiOta() {
    // 外部状态通过 firmwareApp 间接访问（避免新增 extern）
    extern OtaRuntimeState otaRuntime;
    extern WifiRuntimeState wifiRuntime;
    updateWifiOta(otaRuntime, wifiRuntime);
}
#else
static void taskWifiConsole() {}
static void taskWifiWeb() {}
static void taskWifiSta() {}
static void taskCloudReport() {}
static void taskWifiStaHistory() {}
static void taskWifiBootReset() {}
static void taskWifiOta() {}
#endif

static void taskJoystickCal() { updateJoystickCalibration(); }
static void taskSerialRead() {
    extern SerialBuf serial0Buf, serial1Buf;
    extern Stream& Serial2;
    readSerialBuf(Serial, serial0Buf);
    readSerialBuf(Serial1, serial1Buf);
    handleSerial2();
}

// 全局实例
FirmwareApp firmwareApp;

FirmwareApp::FirmwareApp() {
    // 任务表初始化：intervalMs=0 表示每次 loop() 都执行
    tasks[TASK_SENSOR_READ]     = {"sensor",     SENSOR_UPDATE_INTERVAL,  0, taskSensorRead,     true};
    tasks[TASK_JOYSTICK_CAL]    = {"joystick",   0,                       0, taskJoystickCal,    true};
    tasks[TASK_SERIAL_READ]     = {"serial",     0,                       0, taskSerialRead,     true};
    tasks[TASK_WIFI_CONSOLE]    = {"wifi_con",   0,                       0, taskWifiConsole,    true};
    tasks[TASK_WIFI_WEB]        = {"wifi_web",   0,                       0, taskWifiWeb,        true};
    tasks[TASK_WIFI_STA]        = {"wifi_sta",   0,                       0, taskWifiSta,        true};
    tasks[TASK_CLOUD_REPORT]    = {"cloud",      0,                       0, taskCloudReport,    true};
    tasks[TASK_WIFI_STA_HISTORY]= {"sta_hist",   0,                       0, taskWifiStaHistory, true};
    tasks[TASK_WIFI_BOOT_RESET] = {"boot_rst",   0,                       0, taskWifiBootReset,  true};
    tasks[TASK_WIFI_OTA]        = {"wifi_ota",   0,                       0, taskWifiOta,        true};
    tasks[TASK_RC_FILTER]       = {"rc_filter",  RC_FILTER_UPDATE_INTERVAL, 0, taskRcFilter,    true};
    tasks[TASK_TELEMETRY]       = {"telemetry",  0,                       0, taskTelemetry,      true};
}

void FirmwareApp::begin() {
    // 初始化各任务 lastRunMs 为当前时间（避免启动时立即执行）
    uint32_t now = millis();
    for (int i = 0; i < FIRMWARE_TASK_COUNT; i++) {
        tasks[i].lastRunMs = now;
    }
}

void FirmwareApp::update() {
    uint32_t now = millis();
    for (int i = 0; i < FIRMWARE_TASK_COUNT; i++) {
        runTask(tasks[i], now);
    }
}

void FirmwareApp::runTask(FirmwareTask& task, uint32_t nowMs) {
    if (!task.enabled) return;
    if (task.intervalMs == 0 || (uint32_t)(nowMs - task.lastRunMs) >= task.intervalMs) {
        task.callback();
        task.lastRunMs = nowMs;
    }
}

void FirmwareApp::setTaskEnabled(FirmwareTaskId id, bool enabled) {
    if (id < FIRMWARE_TASK_COUNT) tasks[id].enabled = enabled;
}

bool FirmwareApp::isTaskEnabled(FirmwareTaskId id) const {
    return id < FIRMWARE_TASK_COUNT ? tasks[id].enabled : false;
}
