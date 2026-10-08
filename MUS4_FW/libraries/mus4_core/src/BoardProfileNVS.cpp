#include "BoardProfileNVS.h"

// ── Phase 3 P3-3: NVS 运行时切板实现 ──────────────────────────────────────

BoardProfileId BoardProfileNVS::current_profile_ = BOARD_PROFILE_A;
bool BoardProfileNVS::reboot_required_ = false;
bool BoardProfileNVS::initialized_ = false;

void BoardProfileNVS::init() {
    if (initialized_) return;
    
    Preferences prefs;
    if (prefs.begin(BOARD_PROFILE_NVS_NAMESPACE, true)) {
        // NVS 只读模式打开成功
        uint8_t stored = prefs.getUChar(BOARD_PROFILE_NVS_KEY, 0xFF);
        prefs.end();
        
        if (stored == 0xFF) {
            // NVS 无记录，使用编译期默认值
            #ifdef MUS4_BOARD_B
                current_profile_ = BOARD_PROFILE_B;
            #else
                current_profile_ = BOARD_PROFILE_A;
            #endif
        } else if (stored == BOARD_PROFILE_A || stored == BOARD_PROFILE_B) {
            current_profile_ = (BoardProfileId)stored;
        } else {
            // NVS 数据损坏，回落默认值
            #ifdef MUS4_BOARD_B
                current_profile_ = BOARD_PROFILE_B;
            #else
                current_profile_ = BOARD_PROFILE_A;
            #endif
        }
    } else {
        // NVS 打开失败，回落默认值
        #ifdef MUS4_BOARD_B
            current_profile_ = BOARD_PROFILE_B;
        #else
            current_profile_ = BOARD_PROFILE_A;
        #endif
    }
    
    initialized_ = true;
}

BoardProfileId BoardProfileNVS::loadBoardProfile() {
    init();
    return current_profile_;
}

bool BoardProfileNVS::saveBoardProfile(BoardProfileId profile) {
    if (profile != BOARD_PROFILE_A && profile != BOARD_PROFILE_B) {
        return false;
    }
    
    Preferences prefs;
    if (!prefs.begin(BOARD_PROFILE_NVS_NAMESPACE, false)) {
        return false;
    }
    
    bool ok = prefs.putUChar(BOARD_PROFILE_NVS_KEY, (uint8_t)profile);
    prefs.end();
    
    if (ok) {
        current_profile_ = profile;
        reboot_required_ = true;
    }
    
    return ok;
}

BoardProfileId BoardProfileNVS::currentProfile() {
    init();
    return current_profile_;
}

const char* BoardProfileNVS::profileName(BoardProfileId profile) {
    switch (profile) {
        case BOARD_PROFILE_A: return "Board A";
        case BOARD_PROFILE_B: return "Board B";
        default: return "Unknown";
    }
}

bool BoardProfileNVS::isRebootRequired() {
    return reboot_required_;
}

void BoardProfileNVS::clearRebootFlag() {
    reboot_required_ = false;
}

// ── 串口角色访问函数（替代静态引用） ──────────────────────────────────────

HardwareSerial& serialTelemetryPort() {
    BoardProfileId profile = BoardProfileNVS::currentProfile();
    #ifdef MUS4_SWAP_SERIAL0_SERIAL1
        // 编译期对调：Board A 用 USB，Board B 用 TTL
        return (profile == BOARD_PROFILE_A) ? Serial : Serial1;
    #else
        // 编译期默认：Board A 用 TTL，Board B 用 USB
        return (profile == BOARD_PROFILE_A) ? Serial1 : Serial;
    #endif
}

HardwareSerial& serialConsolePort() {
    BoardProfileId profile = BoardProfileNVS::currentProfile();
    #ifdef MUS4_SWAP_SERIAL0_SERIAL1
        return (profile == BOARD_PROFILE_A) ? Serial1 : Serial;
    #else
        return (profile == BOARD_PROFILE_A) ? Serial : Serial1;
    #endif
}
