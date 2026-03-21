/*
 * ESP32 为 Lattepanda MU 配网 - 程序头文件
 * 功能：AP配网模式 + STA工作模式 + 串口交互
 */

#ifndef ESP32_WIFI_PROVISIONING_H
#define ESP32_WIFI_PROVISIONING_H

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// 配置参数
#define WIFI_CONFIG_ADDR       0       // EEPROM起始地址
#define MAX_SSID_LENGTH        32
#define MAX_PASSWORD_LENGTH    64
#define PROVISIONING_TIMEOUT   15000   // 等待MU回复超时(ms)
#define KEY_HOLD_TIME          3000    // 按键长按触发时间(ms)
#define WIFI_CONNECT_TIMEOUT   15000  // MU连接超时(ms)

// 状态枚举
enum DeviceState {
  STATE_STA_WORKING,       // STA模式正常工作
  STATE_AP_PROVISIONING    // AP配网模式
};

// 配网配置存储结构
struct WifiConfig {
  bool valid;               // 配置是否有效
  char ssid[MAX_SSID_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
};

// 全局变量声明
extern DeviceState currentState;
extern WifiConfig savedConfig;
extern WebServer* apServer;
extern String provisioningResultMessage;
extern String muIpAddress;

// 函数声明
void initProvisioning();
bool readConfigFromFlash();
void saveConfigToFlash();
bool checkProvisioningTrigger();
void startApProvisioning();
void stopApProvisioning();
void startStaWorking();
void handleUartCommunication();
void handleWebServer();
void provisioningLoop();
void clearConfig();

#endif // ESP32_WIFI_PROVISIONING_H
