// AuthService.cpp — ESP32 eFuse 芯片 ID 身份识别服务实现
//
// 依赖：
//   - esp_efuse_mac_get_default() 读取 eFuse MAC（6 字节）
//   - Preferences 库（NVS 持久化），命名空间 "auth"，键 "user_id"
//
// 多行协议状态机：
//   AUTH_IDLE ──(CMD:WRITE_UID)──▶ AUTH_WAIT_ARG ──(ARG:...)──▶ AUTH_IDLE
//                                    │
//                                    └──(超时 5s)──▶ AUTH_IDLE + ERR:05

#include "AuthService.h"

#include <Preferences.h>
#include <esp_efuse.h>
#include <esp_mac.h>  // esp_efuse_mac_get_default()

// ---------------------------------------------------------------------------
// 常量
// ---------------------------------------------------------------------------
static const char* AUTH_NVS_NAMESPACE = "auth";
static const char* AUTH_NVS_KEY_USER_ID = "user_id";
static const unsigned long AUTH_ARG_TIMEOUT_MS = 5000;

// ---------------------------------------------------------------------------
// 多行协议状态机
// ---------------------------------------------------------------------------
enum AuthCmdState {
    AUTH_IDLE,       // 等待 CMD:... 行
    AUTH_WAIT_ARG,   // 已收到 CMD:WRITE_UID，等待 ARG:... 行
};

static AuthCmdState g_auth_state = AUTH_IDLE;
static String g_pending_cmd;          // 当前等待参数的命令名
static unsigned long g_auth_deadline_ms = 0;  // 等待参数的截止时间

// ---------------------------------------------------------------------------
// 内部辅助
// ---------------------------------------------------------------------------

/// 从 eFuse 读取 6 字节 MAC，转为小写十六进制字符串（12 字符）。
static String readHwId()
{
    uint8_t mac[6];
    esp_err_t err = esp_efuse_mac_get_default(mac);
    if (err != ESP_OK) {
        // eFuse 读取失败（极罕见），返回空字符串
        return "";
    }

    char buf[13];  // 12 hex chars + null terminator
    snprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

/// 从 NVS 读取 user_id（字符串）。
/// @return 读取到的字符串；未找到或失败时返回空字符串
static String readUserIdFromNvs()
{
    Preferences prefs;
    if (!prefs.begin(AUTH_NVS_NAMESPACE, true)) {
        return "";  // NVS 读取失败
    }

    String uid;
    if (prefs.isKey(AUTH_NVS_KEY_USER_ID)) {
        uid = prefs.getString(AUTH_NVS_KEY_USER_ID, "");
    }
    prefs.end();

    // 读到了空字符串视为未绑定
    uid.trim();
    return uid;
}

/// 写入 user_id 到 NVS。
/// @return true 表示写入成功
static bool writeUserIdToNvs(const String& uid)
{
    Preferences prefs;
    if (!prefs.begin(AUTH_NVS_NAMESPACE, false)) {
        return false;
    }

    size_t written = prefs.putString(AUTH_NVS_KEY_USER_ID, uid);
    prefs.end();

    return written > 0;
}

/// 从 NVS 清除 user_id 键。
/// @return true 表示清除成功
static bool clearUserIdFromNvs()
{
    Preferences prefs;
    if (!prefs.begin(AUTH_NVS_NAMESPACE, false)) {
        return false;
    }

    bool ok = prefs.remove(AUTH_NVS_KEY_USER_ID);
    prefs.end();
    return ok;
}

/// 判断字符串是否为有效的 UUID 格式（36 字符，含 4 个连字符）。
static bool isValidUuidFormat(const String& s)
{
    // UUID v4 格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx（36 字符）
    if (s.length() != 36) return false;
    for (size_t i = 0; i < 36; i++) {
        char c = s[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return false;
        } else {
            if (!isxdigit(c)) return false;
        }
    }
    return true;
}

/// 重置多行协议状态机。
static void resetAuthState()
{
    g_auth_state = AUTH_IDLE;
    g_pending_cmd = "";
    g_auth_deadline_ms = 0;
}

// ---------------------------------------------------------------------------
// 命令处理函数
// ---------------------------------------------------------------------------

static bool handleReadHwId(Print& out)
{
    String hwId = readHwId();
    if (hwId.length() == 0) {
        out.println("ERR:04:eFuse MAC read fail");
    } else {
        out.print("OK:");
        out.println(hwId);
    }
    return true;
}

static bool handleReadUid(Print& out)
{
    String uid = readUserIdFromNvs();
    out.print("OK:");
    // uid 为空时只输出 OK:（表示未绑定）
    if (uid.length() > 0) {
        out.println(uid);
    } else {
        out.println();  // OK:\n
    }
    return true;
}

static bool handleWriteUid(const String& arg, Print& out)
{
    // 校验 UUID 格式
    if (!isValidUuidFormat(arg)) {
        out.println("ERR:02:invalid UUID format");
        return true;
    }

    if (writeUserIdToNvs(arg)) {
        out.println("OK:written");
    } else {
        out.println("ERR:03:NVS write fail");
    }
    return true;
}

static bool handleClearUid(Print& out)
{
    if (clearUserIdFromNvs()) {
        out.println("OK:cleared");
    } else {
        out.println("ERR:03:NVS erase fail");
    }
    return true;
}

// ---------------------------------------------------------------------------
// 公开入口
// ---------------------------------------------------------------------------

bool processAuthCommand(const String& line, Print& out)
{
    // 检查正在等待 ARG 的命令是否超时
    if (g_auth_state == AUTH_WAIT_ARG && millis() > g_auth_deadline_ms) {
        out.println("ERR:05:timeout waiting for argument");
        resetAuthState();
    }

    // 处理 CMD: 前缀的行（新命令）
    if (line.startsWith("CMD:")) {
        // 如果有正在等待 ARG 的旧命令，取消它
        if (g_auth_state == AUTH_WAIT_ARG) {
            out.println("ERR:05:previous command cancelled, new command received");
            resetAuthState();
        }

        String cmd = line.substring(4);
        cmd.trim();

        if (cmd.equalsIgnoreCase("READ_HW_ID")) {
            return handleReadHwId(out);
        }
        if (cmd.equalsIgnoreCase("READ_UID")) {
            return handleReadUid(out);
        }
        if (cmd.equalsIgnoreCase("WRITE_UID")) {
            g_auth_state = AUTH_WAIT_ARG;
            g_pending_cmd = "WRITE_UID";
            g_auth_deadline_ms = millis() + AUTH_ARG_TIMEOUT_MS;
            return true;  // 已消费，等待下一行 ARG
        }
        if (cmd.equalsIgnoreCase("CLEAR_UID")) {
            return handleClearUid(out);
        }

        // 未知命令
        out.println("ERR:01:unknown command");
        return true;
    }

    // 处理 ARG: 前缀的行（正在等待参数的 WRITE_UID 命令的后续）
    if (g_auth_state == AUTH_WAIT_ARG && line.startsWith("ARG:")) {
        String arg = line.substring(4);
        arg.trim();

        bool ok = false;
        if (g_pending_cmd == "WRITE_UID") {
            ok = handleWriteUid(arg, out);
        }
        // 无论成功与否都重置状态
        resetAuthState();
        return ok;
    }

    // 不是 Auth 命令，返回 false 让调用方继续分发
    return false;
}

String getHardwareId()
{
    return readHwId();
}
