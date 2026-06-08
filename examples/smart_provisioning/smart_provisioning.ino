#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

#include "web_ui.h"

static const char* AP_SSID = "ESP32-S3-Provision";
static const char* AP_PASSWORD = "";
static const char* MDNS_HOSTNAME = "esp32";

static const uint16_t HTTP_PORT = 80;
static const uint16_t DNS_PORT = 53;
static const unsigned long STA_CONNECT_TIMEOUT_MS = 18000;
static const unsigned long AP_SHUTDOWN_DELAY_MS = 2500;

WebServer server(HTTP_PORT);
DNSServer dnsServer;

bool apRunning = false;
bool apShutdownScheduled = false;
bool mdnsStarted = false;
unsigned long apShutdownAtMs = 0;
String lastStaIp;

String jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                escaped += ((uint8_t)c < 0x20) ? ' ' : c;
                break;
        }
    }
    return escaped;
}

String extractJsonString(const String& body, const String& key)
{
    String pattern = "\"" + key + "\"";
    int keyIndex = body.indexOf(pattern);
    if (keyIndex < 0) return "";

    int colonIndex = body.indexOf(':', keyIndex + pattern.length());
    if (colonIndex < 0) return "";

    int quoteStart = body.indexOf('"', colonIndex + 1);
    if (quoteStart < 0) return "";

    String result;
    bool escaping = false;
    for (int i = quoteStart + 1; i < body.length(); i++) {
        char c = body[i];
        if (escaping) {
            switch (c) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += c; break;
            }
            escaping = false;
            continue;
        }
        if (c == '\\') {
            escaping = true;
            continue;
        }
        if (c == '"') return result;
        result += c;
    }
    return "";
}

void sendJson(int statusCode, const String& body)
{
    server.sendHeader("Cache-Control", "no-store");
    server.send(statusCode, "application/json", body);
}

String makeErrorJson(const String& error, const String& message)
{
    return String("{\"ok\":false,\"error\":\"") + jsonEscape(error) +
        "\",\"message\":\"" + jsonEscape(message) + "\"}";
}

String wifiStatusToError(wl_status_t status)
{
    switch (status) {
        case WL_NO_SSID_AVAIL: return "no_ssid";
        case WL_CONNECT_FAILED: return "auth_failed";
        case WL_CONNECTION_LOST: return "connection_lost";
        case WL_DISCONNECTED: return "disconnected";
        default: return "timeout";
    }
}

String wifiStatusToMessage(wl_status_t status)
{
    switch (status) {
        case WL_NO_SSID_AVAIL:
            return "未找到目标 SSID，请确认网络名称、距离和 2.4GHz 频段。";
        case WL_CONNECT_FAILED:
            return "认证失败，请检查 Wi-Fi 密码。";
        case WL_CONNECTION_LOST:
            return "连接过程中链路中断，请检查路由器信号。";
        case WL_DISCONNECTED:
            return "STA 未连接，请检查 SSID 和密码。";
        default:
            return "STA 连接超时，请检查 SSID、密码和路由器信号。";
    }
}

void scheduleApShutdown()
{
    apShutdownScheduled = true;
    apShutdownAtMs = millis() + AP_SHUTDOWN_DELAY_MS;
    Serial.printf("[WiFi] 已发送 JSON 响应，将在 %lu ms 后关闭 AP。\n", AP_SHUTDOWN_DELAY_MS);
}

void maybeShutdownAp()
{
    if (!apShutdownScheduled || !apRunning) return;
    if ((long)(millis() - apShutdownAtMs) < 0) return;

    apShutdownScheduled = false;
    Serial.println("[WiFi] 正在关闭 SoftAP，保留 STA 连接...");
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apRunning = false;
    Serial.println("[WiFi] SoftAP 已关闭。");
}

bool waitStaConnected(unsigned long timeoutMs, wl_status_t& finalStatus)
{
    unsigned long startMs = millis();
    while (millis() - startMs < timeoutMs) {
        server.handleClient();
        finalStatus = WiFi.status();
        if (finalStatus == WL_CONNECTED) return true;
        if (finalStatus == WL_NO_SSID_AVAIL || finalStatus == WL_CONNECT_FAILED) return false;
        Serial.print(".");
        delay(250);
    }
    finalStatus = WiFi.status();
    return false;
}

void startMdnsIfNeeded()
{
    if (mdnsStarted) return;
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        mdnsStarted = true;
        Serial.printf("[mDNS] 已启动：http://%s.local/\n", MDNS_HOSTNAME);
    } else {
        Serial.println("[mDNS] 启动失败，esp32.local fallback 可能不可用。");
    }
}

void handleRoot()
{
    server.send_P(200, "text/html; charset=utf-8", SMART_PROVISIONING_HTML);
}

void handleCaptiveProbe()
{
    handleRoot();
}

void handleConfig()
{
    if (server.method() != HTTP_POST) {
        sendJson(405, makeErrorJson("method_not_allowed", "请使用 POST /config。"));
        return;
    }

    String body = server.arg("plain");
    String ssid = extractJsonString(body, "ssid");
    String password = extractJsonString(body, "password");
    ssid.trim();

    if (ssid.length() == 0 || ssid.length() > 32) {
        sendJson(400, makeErrorJson("invalid_ssid", "SSID 不能为空且长度不能超过 32 字节。"));
        return;
    }
    if (password.length() > 0 && (password.length() < 8 || password.length() > 63)) {
        sendJson(400, makeErrorJson("invalid_password", "Wi-Fi 密码为空表示开放网络；非空时长度必须为 8 到 63。"));
        return;
    }

    Serial.println();
    Serial.println("[WiFi] 收到 /config 配网请求。");
    Serial.printf("[WiFi] SSID: %s\n", ssid.c_str());
    Serial.printf("[WiFi] Password: <redacted>, length=%u\n", password.length());
    Serial.println("[WiFi] 开始 STA 连接，请稍候...");

    apShutdownScheduled = false;
    WiFi.disconnect(false, false);
    delay(100);
    if (password.length() == 0) {
        WiFi.begin(ssid.c_str());
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    wl_status_t finalStatus = WL_IDLE_STATUS;
    bool connected = waitStaConnected(STA_CONNECT_TIMEOUT_MS, finalStatus);
    Serial.println();

    if (!connected) {
        String error = wifiStatusToError(finalStatus);
        String message = wifiStatusToMessage(finalStatus);
        Serial.printf("[WiFi] STA 连接失败：%s，status=%d\n", error.c_str(), (int)finalStatus);
        sendJson(408, makeErrorJson(error, message));
        return;
    }

    lastStaIp = WiFi.localIP().toString();
    Serial.printf("[WiFi] STA 已连接，IP: %s\n", lastStaIp.c_str());
    startMdnsIfNeeded();

    String redirect = "http://" + lastStaIp + "/";
    String mdns = String("http://") + MDNS_HOSTNAME + ".local/";
    String response = String("{\"ok\":true,\"ip\":\"") + jsonEscape(lastStaIp) +
        "\",\"redirect\":\"" + jsonEscape(redirect) +
        "\",\"mdns\":\"" + jsonEscape(mdns) + "\"}";
    sendJson(200, response);
    scheduleApShutdown();
}

void handleNotFound()
{
    handleRoot();
}

void setupRoutes()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/generate_204", HTTP_GET, handleCaptiveProbe);
    server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveProbe);
    server.on("/connecttest.txt", HTTP_GET, handleCaptiveProbe);
    server.on("/ncsi.txt", HTTP_GET, handleCaptiveProbe);
    server.onNotFound(handleNotFound);
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("[Boot] ESP32-S3 智能配网示例启动。");
    Serial.println("[Boot] 注意：ESP32-S3 仅支持 2.4GHz Wi-Fi。");

    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);

    bool apStarted = strlen(AP_PASSWORD) >= 8 ? WiFi.softAP(AP_SSID, AP_PASSWORD) : WiFi.softAP(AP_SSID);
    if (!apStarted) {
        Serial.println("[WiFi] SoftAP 启动失败。");
        return;
    }

    apRunning = true;
    IPAddress apIp = WiFi.softAPIP();
    Serial.printf("[WiFi] SoftAP SSID: %s\n", AP_SSID);
    Serial.printf("[WiFi] SoftAP IP: %s\n", apIp.toString().c_str());

    dnsServer.start(DNS_PORT, "*", apIp);
    Serial.println("[DNS] Captive Portal DNS 已启动。");

    setupRoutes();
    server.begin();
    Serial.printf("[HTTP] WebServer 已启动：http://%s/\n", apIp.toString().c_str());
}

void loop()
{
    if (apRunning) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
    maybeShutdownAp();
    delay(2);
}
