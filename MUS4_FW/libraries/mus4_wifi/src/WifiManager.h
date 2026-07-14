#pragma once

#include "FirmwareConfig.h"

#ifdef ENABLE_WIFI_CONSOLE

#include <Arduino.h>
#include <WiFi.h>
#include "WifiConsoleTypes.h"

void loadDevModePreference();
bool saveDevModePreference(bool enabled);
void loadJudgeConfigPreference();
bool saveJudgeConfigPreference(const JudgeConfig& config);
bool resetJudgeConfigPreference();
void loadDriftConfigPreference();
bool saveDriftConfigPreference(const DriftConfig& config);
bool resetDriftConfigPreference();

void startWifiMdnsIfNeeded();
void stopWifiMdnsIfNeeded();

#ifdef ENABLE_WIFI_NETBIOS_DISCOVERY
void startWifiNetbiosIfNeeded();
void stopWifiNetbiosIfNeeded();
#endif

#ifdef ENABLE_WIFI_LLMNR_DISCOVERY
void startWifiLlmnrIfNeeded();
void stopWifiLlmnrIfNeeded();
#endif

void clearWifiStaHandoff();
void finishWifiStaHandoff();
void startWifiStaHandoff(const String& targetSsid);
void disconnectWifiStaOnly();
void applyWifiStaCredentials();
void scheduleWifiApRestart();
bool ensureWifiApAvailable();
bool restartWifiAp();
bool clearWifiStaAndRestoreAp();
void updateWifiBootResetButton();

void loadWifiApPreference();
bool saveWifiApPreference(const String& ssid);
String getActiveWifiApSsid();

void setupWifiConsole();
void updateWifiSta();
void updateWifiConsole();

void setupWifiWebConsole();
void updateWifiWebConsole();

#endif // ENABLE_WIFI_CONSOLE
