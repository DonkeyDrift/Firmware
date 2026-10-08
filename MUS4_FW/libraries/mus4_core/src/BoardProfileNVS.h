#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "BoardProfile.h"

// ── Phase 3 P3-3: NVS 运行时切板 ──────────────────────────────────────────
// 单镜像免重刷伺候两块主控板：NVS 存板型选择，boot 时读取并绑定串口角色。
//
// 设计文档：docs/Plan/主控板档案-NVS运行时切换方案.md
//
// 优先级：NVS > BoardProfile.local.h > 编译期默认（BoardProfile.h）

#define BOARD_PROFILE_NVS_KEY "board_profile"
#define BOARD_PROFILE_NVS_NAMESPACE "mus4_board"

enum BoardProfileId {
    BOARD_PROFILE_A = 0,  // 当前主控板（默认）
    BOARD_PROFILE_B = 1,  // 另一块主控板
};

class BoardProfileNVS {
public:
    // 从 NVS 读取板型（失败返回编译期默认值）
    static BoardProfileId loadBoardProfile();
    
    // 保存板型到 NVS
    static bool saveBoardProfile(BoardProfileId profile);
    
    // 获取当前板型（运行时）
    static BoardProfileId currentProfile();
    
    // 获取板型名称（用于 banner/Web Console）
    static const char* profileName(BoardProfileId profile);
    
    // 检查是否需要重启（切换后返回 true）
    static bool isRebootRequired();
    
    // 清除重启标志（boot 后调用）
    static void clearRebootFlag();
    
private:
    static BoardProfileId current_profile_;
    static bool reboot_required_;
    static bool initialized_;
    
    static void init();
};

// 便捷访问函数
HardwareSerial& serialTelemetryPort();
HardwareSerial& serialConsolePort();
