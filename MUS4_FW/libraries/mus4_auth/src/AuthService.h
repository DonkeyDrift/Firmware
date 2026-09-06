#pragma once
// AuthService.h — ESP32 eFuse 芯片 ID 身份识别服务
//
// 通过串口提供以下命令（文本帧，\\n 分隔）：
//   CMD:READ_HW_ID  → OK:<12-char-lowercase-hex>
//   CMD:READ_UID    → OK:<uuid> 或 OK:（未绑定时为空）
//   CMD:WRITE_UID   →（等待下一行 ARG:<uuid>）→ OK:written 或 ERR:03:...
//   CMD:CLEAR_UID   → OK:cleared 或 ERR:03:...
//
// 错误码：
//   01 — 未知命令
//   02 — 无效参数
//   03 — NVS 写入/擦除失败
//   04 — NVS 读取失败
//   05 — 等待参数超时

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 处理一行 Auth 命令。
/// @param line 输入行（不含 \\r 和 \\n）
/// @param out  回复输出的 Print 目标（通常是 Serial1 或 Serial）
/// @return true 表示该行已被 Auth 服务消费，无需继续分发
bool processAuthCommand(const String& line, Print& out);

/// 读取 eFuse MAC 派生的 12 位小写十六进制硬件 ID 字符串。
/// @return 12 字符硬件 ID；eFuse 读取失败时返回空字符串
String getHardwareId();

#ifdef __cplusplus
}
#endif
