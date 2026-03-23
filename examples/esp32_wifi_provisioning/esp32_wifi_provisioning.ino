/*
 * ESP32 为 Lattepanda MU 配网 - 主程序入口
 * 基于Arduino ESP32框架
 * 
 * 硬件连接：
 * - 配网按键：BOOT按钮 (GPIO0)，自带下拉，无需额外电阻
 * - 串口连接MU：ESP32 TX -> MU RX, ESP32 RX -> MU TX
 * 
 * 串口通信协议：
 * - ESP32 -> MU: WIFI|SSID|PASSWORD\n
 * - MU -> ESP32: OK:192.168.1.100\n  (联网成功返回IP)
 * - MU -> ESP32: FAIL\n  (联网失败)
 */

#include "esp32_wifi_provisioning.h"
#define UART_SEL 12
#define DEBUG_SerialTransparent

void setup() {

  pinMode(UART_SEL, OUTPUT);
  digitalWrite(UART_SEL, LOW);
  
  // 初始化串口（波特率根据实际情况调整）
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
  delay(10);

  debugPrintln("[ESP32] 系统启动");
  
  // 初始化配网系统
  initProvisioning();
  
  // 这里初始化其他外设（传感器、舵机等）
  // initHardware();
}

void loop() {
  #ifdef DEBUG_SerialTransparent
  // 注释掉这里的 Serial1 读取，否则会把配网协议数据吃掉，导致状态机收不到 OK:IP
  // while (Serial1.available()) {
  //   Serial.write(Serial1.read());
  // }
  while (Serial.available()) {
    Serial1.write(Serial.read());
  }
  #endif

  // 配网状态机处理
  provisioningLoop();
  
  // 如果是STA工作模式，运行你的主逻辑
  if (currentState == STATE_STA_WORKING) {
    // 这里放你的数据采集、控制代码
    // doYourWork();
  }
  
  delay(1);
}
