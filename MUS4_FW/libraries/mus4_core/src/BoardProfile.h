#pragma once

// ── 主控板通信档案：两块主控板的串口角色差异，唯一定义点 ─────────────────────
// 背景：两块主控板（上位机）与本 ESP32 的串口接线不同，宏 MUS4_SWAP_SERIAL0_SERIAL1
// 决定 Serial0（USB Type-C / UART0）与 Serial1（TTL RX1=16/TX1=17）谁当"主遥测
// 端口"、谁当"控制台端口"。角色映射的唯一实现点在 SerialRole.h，本文件只负责
// "选哪套配置"。
//
// 【选择优先级：命令行宏 > 本地档案 > 默认板 A】
//   1) 构建期选择（不改任何入库文件，零 git diff）：
//        python3 arduino-cli.py -c                 ← 板 A（当前主控板，默认）
//        python3 arduino-cli.py -c -D MUS4_BOARD_B ← 板 B（另一块主控板）
//   2) 本机私有微调：libraries/mus4_core/src/BoardProfile.local.h（gitignored，
//      与 WirelessSecrets.h 同款模式；只能覆盖，不能承载必需配置）
//   3) 默认：板 A，行为与 v1.10.13 起的固件完全一致。
//
// ⚠️ 工程纪律（v1.8.77 事故，见 FirmwareConfig.h 注释）：必需配置的默认值必须
//    留在本 tracked 文件里——曾经把云端上报默认值放进 gitignored 文件，导致干净
//    clone / 新 worktree 里整块代码被静默编译掉（车在线却在网页上查不到）。

#if defined(__has_include)
#if __has_include("BoardProfile.local.h")
#include "BoardProfile.local.h"   // 本机覆盖（gitignored）
#endif
#endif

// 没有指定任何板型时 = 板 A（当前主控板）
#if !defined(MUS4_BOARD_A) && !defined(MUS4_BOARD_B)
#define MUS4_BOARD_A 1
#endif

#if defined(MUS4_BOARD_A) && defined(MUS4_BOARD_B)
#error "MUS4_BOARD_A 与 MUS4_BOARD_B 只能选其一：检查 -D 参数或 BoardProfile.local.h"
#endif

#ifdef MUS4_BOARD_A
// ── 板 A：当前主控板（出厂默认，与 v1.10.13 起的线上行为一致）────────────────
//   主遥测（上行 T..S../M:P/$IMU + 下行 <t>:<s> 控制帧）→ USB Type-C（Serial0）
//   日志 / ANSI TUI / 本地命令控制台                    → TTL RX1=16/TX1=17（Serial1）
#define MUS4_SWAP_SERIAL0_SERIAL1
#else
// ── 板 B：另一块主控板（-DMUS4_BOARD_B 编译）────────────────────────────────
//   主遥测 → TTL RX1=16/TX1=17（Serial1）；控制台 → USB Type-C（Serial0）
//   即"不定义 MUS4_SWAP_SERIAL0_SERIAL1"的原布局。
//   注：若两块板的接线与上述描述相反，交换本文件两个分支的宏即可（只改这一处）。
//   未来板型差异（波特率、协议版本、帧格式开关）一律在对应分支内定义。
#endif
