#pragma once
#include <Arduino.h>

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE

// 上位机配网状态（Serial2 STATUS|/OK|/FAIL|/HOSTIP| 帧）。
// 主 loop（handleSerial2）写、Web 任务读——固定 char 数组而非 String：
// 跨任务共享可变 static String 会并发 realloc 撕堆（v1.7.16/17 前科），
// POD 缓冲按字段原子可见。缓冲定义在 WebConsoleServer.cpp。
extern char hostWifiStatus[32];
extern char hostWifiSsid[40];
extern char hostWifiIp[20];
extern char hostWifiError[80];
extern char hostReportedIp[20];
extern unsigned long hostReportedIpMs;

// Register all HTTP routes for the Web Console and start the WebServer.
// v1.10.17 起：本函数末尾同时创建 wifi_web 任务驱动 handleClient()（创建失败
// 时回退主 loop 直驱，见 FirmwareConfig.h WIFI_WEB_TASK_* 注释）。
void setupWebConsoleServer();

// Process one update tick for the Web Console: sample telemetry data,
// record handler timing statistics.
// Does NOT handle WebSocket (see WebTelemetry), AP restart, or DNS server.
// v1.10.17 起：HTTP 的 handleClient() 不再在这里驱动（移至 wifi_web 任务），
// 本函数只保留主 loop 侧的遥测采样与节奏统计。
void updateWebConsoleServer();

#endif
