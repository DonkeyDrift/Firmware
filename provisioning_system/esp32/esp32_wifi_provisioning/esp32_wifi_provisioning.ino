#include <WiFi.h>
#include <WebServer.h>
#include "web_ui.h" // 包含预定义的HTML字符串

#define AP_SSID "MUS4-AP"
#define UART_SEL_PIN 12

// 全局配网状态
String currentProvStatus = "IDLE";

// 实例化 WebServer (端口 80)
WebServer server(80);

// URL解码辅助函数
String urldecode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == '+') {
            encodedString += ' ';
        } else if (c == '%') {
            i++;
            code0 = str.charAt(i);
            i++;
            code1 = str.charAt(i);
            c = (char)strtol((String(code0) + String(code1)).c_str(), NULL, 16);
            encodedString += c;
        } else {
            encodedString += c;
        }
    }
    return encodedString;
}

// 1. 提供主页 HTML
void handleRoot() {
    server.send(200, "text/html", web_ui_html);
}

// 2. 接收配网表单 (POST)
void handleProvision() {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    if (server.hasArg("ssid") && server.hasArg("pwd")) {
        String ssid = urldecode(server.arg("ssid"));
        String pwd = urldecode(server.arg("pwd"));
        
        // 构造发送给 Linux 的串口指令
        String cmd = "WIFI|" + ssid + "|" + pwd + "\n";
        
        // 发送给 MU
        Serial1.print(cmd);
        Serial.println("[ESP32] 发送配网指令: " + cmd);
        
        // 更新内部状态
        currentProvStatus = "STATUS|CONNECTING";
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "FAIL");
    }
}

// 3. 查询当前状态 (GET)
void handleStatus() {
    server.send(200, "text/plain", currentProvStatus);
}

void setup() {
    // 串口0 (调试输出)
    Serial.begin(115200);
    // 串口1 (与 MU 通信, TX:17, RX:16)
    Serial1.begin(115200, SERIAL_8N1, 16, 17);
    
    // 如果有UART多路复用引脚控制
    pinMode(UART_SEL_PIN, OUTPUT);
    digitalWrite(UART_SEL_PIN, LOW);
    
    delay(100);
    Serial.println("\n[ESP32] Arduino 配网系统启动");

    // 配置 WiFi AP 模式
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID); // 无密码，方便用户连接
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("[ESP32] AP IP 地址: ");
    Serial.println(IP);

    // 注册 HTTP 路由
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/provision", HTTP_POST, handleProvision);
    server.on("/api/status", HTTP_GET, handleStatus);
    
    server.begin();
    Serial.println("[ESP32] HTTP WebServer 已启动");
}

void loop() {
    // 1. 处理 WebServer 请求
    server.handleClient();
    
    // 2. 处理 Linux 主机通过串口返回的状态信息
    if (Serial1.available()) {
        String line = Serial1.readStringUntil('\n');
        line.trim(); // 移除首尾空白和换行
        
        if (line.length() > 0) {
            Serial.println("[ESP32] 收到MU状态回复: " + line);
            // 预期收到 "OK|192.168.x.x" 或 "FAIL|Reason"
            if (line.startsWith("OK|") || line.startsWith("FAIL|") || line.startsWith("STATUS|")) {
                currentProvStatus = line;
            }
        }
    }
    
    delay(10);
}
