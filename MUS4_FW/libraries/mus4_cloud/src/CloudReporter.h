#pragma once
// CloudReporter.h — 「找小车」云端心跳上报
//
// ESP32 连上家里 Wi-Fi 拿到 IP 后立即向 Cloudflare Pages Functions 上报一次，
// 之后每 5 分钟心跳上报一次自己的局域网 IP，供网页「找 Donkey Car」查询；
// 尚未成功上报过时按退避阶梯重试（60s→120s→300s 封顶），DHCP 换 IP 时立即
// 补报（补报同样受低堆门限保护）。
//
// v1.10.17 内存语义（回归修复后，详见 .cpp 头注释）：TLS 会话「纪元」常驻
// ——会话对象任务级构造、心跳经 HTTP keep-alive 复用（稳态每跳 ~0KB 新分配）；
// 任一跳失败立即 client.stop() 整体释放进入冷态；冷态重建须过 DRAM(8BIT) 池
// 双门限（free8≥48KB 且 maxAlloc8≥17KB），不达标零分配跳过。上一轮的「按次
// 建连」已被实测证伪：8BIT 池稳态仅 ~50.7KB，而一次 TLS 尝试峰值 ~48.8KB，
// 仅 ~1.9KB 余量（本 core 无缓冲缩减 API），重试节拍上曾把池打到 108 字节并
// 引发 PANIC。上报仍在独立 core0 worker 任务（16KB 栈），每次上报实测堆水位
// 与任务栈高水位经 "cloud" 日志回传（heap=/min_heap=/min_heap8=/max_blk=/
// stack_hwm=/next=/fails=）。
//
// 退避：尚未成功上报过时按 60s→120s→300s 阶梯重试（低堆跳过计入同一阶梯），
// 首次成功后恢复 5 分钟心跳节奏。
//
// 默认关闭：需在 FirmwareConfig.h 打开 ENABLE_CLOUD_REPORT。上报地址默认取
// FirmwareConfig.h 的 CLOUD_REPORT_URL_DEFAULT（公开端点），可在本机
// WirelessSecrets.h 里用 CLOUD_REPORT_URL 覆盖。

#include <Arduino.h>

namespace mus4cloud {

/// 每循环调用一次；未启用（ENABLE_CLOUD_REPORT 未打开）时为空操作。
void update();

/// 是否已启用云端上报（编译期开关）。
bool isConfigured();

}  // namespace mus4cloud
