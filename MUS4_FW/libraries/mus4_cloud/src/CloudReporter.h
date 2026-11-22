#pragma once
// CloudReporter.h — 「找小车」云端心跳上报
//
// ESP32 连上家里 Wi-Fi 拿到 IP 后立即向 Cloudflare Pages Functions 上报一次，
// 之后每 5 分钟心跳上报一次自己的局域网 IP，供网页「找 Donkey Car」查询；
// DHCP 换 IP 时立即补报（同样受低堆门限保护）。
//
// v1.10.17 最终内存/连接语义：TLS 对象只在 worker 任务级构造一次，但**每一跳
// 无论成败都在响应后 client.stop() 整体释放**——实车证明旧 keep-alive 会话在
// 下一跳稳定 read timeout(-11)，而新建会话的快速重试可返回 200。冷态建连走
// DRAM(8BIT) 双门限：开机早期/曾成功后的重连用 free8≥40KB、maxAlloc8≥34KB，
// 从未成功过的后期冷启动用 free8≥64KB、maxAlloc8≥49KB；不达标零分配跳过。
// 连续失败按 60s→120s→300s 退避，成功后首次稳态失败允许 15s 快速重试。
// 上报仍在独立 core0 worker（栈按实测高水位收敛为 8KB），每次回传堆水位、
// 最大连续块、栈高水位与下次间隔（heap=/min_heap8=/max_blk=/stack_hwm=/next=）。
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
