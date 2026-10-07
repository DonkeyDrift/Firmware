#pragma once
#include <Arduino.h>
#include "FirmwareConfig.h"

// ── 串口角色映射（Serial0/Serial1 对调开关的单一实现点）─────────────────────
// 两个"角色"贯穿整个固件：
//   serialTelemetry —— 主遥测端口：上行 T..S.. / M:P / $IMU，下行 <t>:<s> 控制帧。
//                      默认绑定 Serial1（TTL RX1=16/TX1=17）。
//   serialConsole   —— 控制台端口：mus4Log 的 SERIAL 目标输出、TUI 仪表盘、
//                      本地命令行。默认绑定 Serial0（USB Type-C / UART0）。
// FirmwareConfig.h 定义 MUS4_SWAP_SERIAL0_SERIAL1 时两者对调：主遥测走 USB
// Type-C，控制台走 TTL（需要从 USB Type-C 口输出主遥测信息时启用该宏）。
// 硬件初始化（begin/引脚/波特率）不随角色变化，仅逻辑角色互换。
// 引用定义在 MUS4_FW.ino（主编译单元），此处仅声明。
#ifdef MUS4_SWAP_SERIAL0_SERIAL1
extern HardwareSerial& serialTelemetry;  // = Serial  (USB Type-C / UART0)
extern HardwareSerial& serialConsole;    // = Serial1 (TTL RX1=16/TX1=17)
#else
extern HardwareSerial& serialTelemetry;  // = Serial1 (TTL RX1=16/TX1=17)
extern HardwareSerial& serialConsole;    // = Serial  (USB Type-C / UART0)
#endif

// WebLog 源标签按"角色"而非物理口归类：主遥测端口恒记为 "serial1"（命中
// WebLogBuffer 里 SERIAL1 专用高吞吐环形缓冲），控制台端口恒记为 "serial"。
// 物理口对调后标签保持稳定，避免高速遥测/控制帧在对调模式下涌入通用 64 槽
// 日志环、挤掉 web/tcp/cmd 等一般日志。
inline const char* serialRoleSourceFor(HardwareSerial& ser)
{
    return &ser == &serialTelemetry ? "serial1" : "serial";
}
