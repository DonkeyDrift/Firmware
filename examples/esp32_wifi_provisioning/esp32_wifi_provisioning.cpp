/*
 * ESP32 为 Lattepanda MU 配网 - 程序实现
 * 基于Arduino ESP32框架
 */

#include "esp32_wifi_provisioning.h"

// 全局变量定义
DeviceState currentState;
WifiConfig savedConfig;
WebServer* apServer;
String provisioningResultMessage;
String muIpAddress;

// 引脚定义（根据硬件调整）
#define PROVISIONING_PIN   0    // BOOT按键，可修改

// 内部函数声明
void handleRoot();
void handleConfigSubmit();
void sendConfigToMU();
bool readUartLine(String &line);

// 调试打印函数：同时向Serial和Serial1输出
void debugPrint(const String &msg) {
  Serial.print(msg);
  Serial1.print(msg);
}

void debugPrintln(const String &msg) {
  Serial.println(msg);
  Serial1.println(msg);
}

/*
 * 初始化配网系统
 */
void initProvisioning() {
  debugPrintln("[ESP32] 初始化配网系统...");

  // 初始化EEPROM
  EEPROM.begin(512);
  
  // 初始化按键
  pinMode(PROVISIONING_PIN, INPUT_PULLUP);
  
  currentState = STATE_AP_PROVISIONING;
  provisioningResultMessage = "";
  muIpAddress = "";
  
  // 读取已有配置
  bool hasConfig = readConfigFromFlash();
  debugPrintln("[ESP32] 配置检查: " + String(hasConfig ? "有配置" : "无配置"));
  
  // 检查是否需要触发配网
  if (!checkProvisioningTrigger()) {
    // 不需要触发配网，检查已有配置
    if (hasConfig) {
      debugPrintln("[ESP32] 已有WiFi配置: " + String(savedConfig.ssid));
      // 检查串口是否就绪
      if (Serial) {
        debugPrintln("[ESP32] 串口就绪，发送配置给MU...");
        // 串口就绪，发送配置给MU
        sendConfigToMU();
        // 等待回复，超时自动进入AP
        unsigned long start = millis();
        bool gotReply = false;
        while (millis() - start < PROVISIONING_TIMEOUT) {
          String line;
          if (readUartLine(line)) {
            if (line.startsWith("OK:")) {
              // 成功，提取IP
              muIpAddress = line.substring(3);
              provisioningResultMessage = "联网成功，MU IP: " + muIpAddress;
              debugPrintln("[ESP32] MU联网成功，IP: " + muIpAddress);
              currentState = STATE_STA_WORKING;
              stopApProvisioning();
              startStaWorking();
              gotReply = true;
              break;
            } else if (line.startsWith("FAIL")) {
              // 失败，进入配网
              provisioningResultMessage = "MU联网失败，请重新配置";
              debugPrintln("[ESP32] MU联网失败");
              gotReply = true;
              break;
            }
          }
          delay(10);
        }
        if (!gotReply) {
          provisioningResultMessage = "等待MU回复超时，进入配网模式";
          debugPrintln("[ESP32] 等待MU回复超时，进入配网模式");
        }
      } else {
        // 串口未就绪，直接进入STA工作
        debugPrintln("[ESP32] 串口未就绪，直接进入STA工作模式");
        currentState = STATE_STA_WORKING;
        startStaWorking();
      }
    }
  } else {
    provisioningResultMessage = "手动触发配网";
    debugPrintln("[ESP32] 手动触发配网模式");
  }
  
  // 如果还是配网模式，启动AP
  if (currentState == STATE_AP_PROVISIONING) {
    debugPrintln("[ESP32] 启动AP配网模式");
    startApProvisioning();
  }
}

/*
 * 检查是否需要触发配网模式
 * 返回 true = 需要进入配网
 */
bool checkProvisioningTrigger() {
  // 检查按键长按
  if (digitalRead(PROVISIONING_PIN) == LOW) {
    delay(KEY_HOLD_TIME);
    if (digitalRead(PROVISIONING_PIN) == LOW) {
      // 长按超过3秒，强制进入配网
      return true;
    }
  }
  
  // 按键没长按，看配置是否存在
  if (!savedConfig.valid) {
    // 没有配置，必须配网
    return true;
  }
  
  // 已有配置，不需要触发
  return false;
}

/*
 * 从Flash读取配置
 */
bool readConfigFromFlash() {
  savedConfig.valid = EEPROM.read(WIFI_CONFIG_ADDR);
  if (savedConfig.valid != 0xAA) {
    savedConfig.valid = false;
    return false;
  }
  
  // 读取SSID
  for (int i = 0; i < MAX_SSID_LENGTH; i++) {
    savedConfig.ssid[i] = EEPROM.read(WIFI_CONFIG_ADDR + 1 + i);
  }
  // 读取密码
  for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
    savedConfig.password[i] = EEPROM.read(WIFI_CONFIG_ADDR + 1 + MAX_SSID_LENGTH + i);
  }
  
  savedConfig.valid = true;
  return true;
}

/*
 * 保存配置到Flash
 */
void saveConfigToFlash() {
  EEPROM.write(WIFI_CONFIG_ADDR, 0xAA);  // 有效标记
  
  // 保存SSID
  for (int i = 0; i < MAX_SSID_LENGTH; i++) {
    EEPROM.write(WIFI_CONFIG_ADDR + 1 + i, savedConfig.ssid[i]);
  }
  // 保存密码
  for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
    EEPROM.write(WIFI_CONFIG_ADDR + 1 + MAX_SSID_LENGTH + i, savedConfig.password[i]);
  }
  
  EEPROM.commit();
}

/*
 * 清除配置（重置用）
 */
void clearConfig() {
  EEPROM.write(WIFI_CONFIG_ADDR, 0x00);
  EEPROM.commit();
  savedConfig.valid = false;
}

/*
 * 启动AP配网模式
 */
void startApProvisioning() {
  WiFi.softAP("MUS4-AP", "");  // 开放网络，无需密码
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), 
                    IPAddress(192, 168, 4, 1), 
                    IPAddress(255, 255, 255, 0));
  
  apServer = new WebServer(80);
  apServer->on("/", handleRoot);
  apServer->on("/submit", handleConfigSubmit);
  apServer->begin();
  
  currentState = STATE_AP_PROVISIONING;
}

/*
 * 停止AP配网模式
 */
void stopApProvisioning() {
  if (apServer != nullptr) {
    apServer->close();
    delete apServer;
    apServer = nullptr;
  }
  WiFi.softAPdisconnect(true);
}

/*
 * 启动STA工作模式
 */
void startStaWorking() {
  // ESP32作为STA不需要连接WiFi，直接启动功能
  // 这里初始化你的数据采集/上传脚本即可
  currentState = STATE_STA_WORKING;
}

/*
 * 发送WiFi配置给MU
 */
void sendConfigToMU() {
  if (!Serial) return;
  
  // 协议格式：WIFI|SSID|PASSWORD\r\n
  Serial.print("WIFI|");
  Serial.print(savedConfig.ssid);
  Serial.print("|");
  Serial.println(savedConfig.password);
}

/*
 * 处理串口通信
 * 应该在loop()中定期调用
 */
void handleUartCommunication() {
  if (!Serial) return;
  
  String line;
  if (readUartLine(line)) {
    if (line.startsWith("OK:")) {
      // 成功
      muIpAddress = line.substring(3);
      provisioningResultMessage = "联网成功，MU IP: " + muIpAddress;
      if (currentState == STATE_AP_PROVISIONING) {
        stopApProvisioning();
        currentState = STATE_STA_WORKING;
        startStaWorking();
      }
    } else if (line.startsWith("FAIL")) {
      // 失败
      provisioningResultMessage = "MU联网失败，请检查WiFi信息重试";
    }
  }
}

/*
 * 处理Web服务器（AP模式下在loop调用）
 */
void handleWebServer() {
  if (currentState == STATE_AP_PROVISIONING && apServer != nullptr) {
    apServer->handleClient();
  }
}

/*
 * 读取串口一行
 */
bool readUartLine(String &line) {
  line = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      return line.length() > 0;
    }
    if (c != '\r') {
      line += c;
    }
  }
  return false;
}

/*
 * Web根页面处理
 */
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MUS4 配网</title>
  <style>
    body { font-family: Arial; max-width: 600px; margin: 30px auto; padding: 20px; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; }
    input { width: 100%; padding: 8px; font-size: 16px; }
    button { background: #007bff; color: white; border: none; padding: 10px 20px; font-size: 16px; cursor: pointer; }
    .message { margin: 15px 0; padding: 10px; border-radius: 4px; }
    .success { background: #d4edda; color: #155724; }
    .error { background: #f8d7da; color: #721c24; }
  </style>
</head>
<body>
  <h1>MUS4 配网设置</h1>
  <p>请输入要连接的WiFi信息，配置完成后MU会自动联网。</p>
  )";
  
  if (provisioningResultMessage.length() > 0) {
    if (provisioningResultMessage.startsWith("联网成功")) {
      html += "<div class=\"message success\">" + provisioningResultMessage + "</div>";
    } else {
      html += "<div class=\"message error\">" + provisioningResultMessage + "</div>";
    }
  }
  
  html += R"(
  <form action="/submit" method="post">
    <div class="form-group">
      <label>WiFi SSID (网络名称)</label>
      <input type="text" name="ssid" required>
    </div>
    <div class="form-group">
      <label>WiFi 密码</label>
      <input type="password" name="password" required>
    </div>
    <button type="submit">提交配置</button>
  </form>
</body>
</html>
  )";
  
  apServer->send(200, "text/html", html);
}

/*
 * 处理配置提交
 */
void handleConfigSubmit() {
  if (!apServer->hasArg("ssid") || !apServer->hasArg("password")) {
    apServer->send(400, "text/plain", "参数错误");
    return;
  }
  
  String ssid = apServer->arg("ssid");
  String password = apServer->arg("password");
  
  // 保存到配置结构
  memset(&savedConfig, 0, sizeof(savedConfig));
  ssid.toCharArray(savedConfig.ssid, MAX_SSID_LENGTH);
  password.toCharArray(savedConfig.password, MAX_PASSWORD_LENGTH);
  savedConfig.valid = true;
  
  // 保存到Flash
  saveConfigToFlash();
  
  // 如果串口就绪，发送给MU
  if (Serial) {
    sendConfigToMU();
    provisioningResultMessage = "配置已保存，正在等待MU联网...";
  } else {
    provisioningResultMessage = "配置已保存，但串口未就绪，请检查MU";
  }
  
  // 重定向回首页
  apServer->send(200, "text/html", R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="3;url=/">
</head>
<body>
  <h1>配置已提交</h1>
  <p>3秒后自动返回首页...</p>
</body>
</html>
  )");
}

/*
 * 主loop应该调用这个函数
 */
void provisioningLoop() {
  handleUartCommunication();
  handleWebServer();
}
