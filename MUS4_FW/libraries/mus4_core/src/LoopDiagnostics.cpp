#include "LoopDiagnostics.h"

#include <esp_system.h>
#include <esp_task_wdt.h>

static portMUX_TYPE s_loopDiagMux = portMUX_INITIALIZER_UNLOCKED;
static LoopDiagnosticsSnapshot s_loopDiag;
static unsigned long s_lastFeedMs = 0;
static bool s_loopDiagArmed = false;

void loopDiagnosticsBegin()
{
    if (s_loopDiagArmed) return;
#if LOOP_TASK_WDT_ENABLED
    // TWDT 本体通常已由 core sdkconfig 初始化（INIT=y、超时 5s、panic）。
    // esp_task_wdt_init 返回 ESP_ERR_INVALID_STATE 表示已初始化——保持 sdkconfig
    // 的既有超时与 idle 订阅不动，只补上 loopTask 自己的订阅。
    esp_task_wdt_config_t config = {};
    config.timeout_ms = LOOP_TASK_WDT_TIMEOUT_S * 1000UL;
    config.idle_core_mask = 0;
    config.trigger_panic = true;
    esp_err_t err = esp_task_wdt_init(&config);
    if (err == ESP_ERR_INVALID_STATE) {
        err = ESP_OK;
    }
    if (err == ESP_OK) {
        err = esp_task_wdt_add(NULL);
    }
    s_loopDiagArmed = (err == ESP_OK);
#else
    s_loopDiagArmed = false;
#endif
}

void loopDiagnosticsFeed()
{
    unsigned long now = millis();
    uint32_t dt = s_lastFeedMs == 0 ? 0 : (uint32_t)(now - s_lastFeedMs);
    s_lastFeedMs = now;
#if LOOP_TASK_WDT_ENABLED
    if (s_loopDiagArmed) {
        esp_task_wdt_reset();
    }
#endif
    portENTER_CRITICAL(&s_loopDiagMux);
    s_loopDiag.lastDtMs = dt;
    if (dt > s_loopDiag.maxDtMs) {
        s_loopDiag.maxDtMs = dt;
    }
    s_loopDiag.frames++;
    portEXIT_CRITICAL(&s_loopDiagMux);
}

void loopDiagnosticsSnapshot(LoopDiagnosticsSnapshot& out)
{
    portENTER_CRITICAL(&s_loopDiagMux);
    out = s_loopDiag;
    portEXIT_CRITICAL(&s_loopDiagMux);
}

void espResetReasonText(char* buf, size_t len)
{
    if (!buf || len == 0) return;
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_UNKNOWN:    strlcpy(buf, "UNKNOWN", len); break;
        case ESP_RST_POWERON:    strlcpy(buf, "POWERON", len); break;
        case ESP_RST_SW:         strlcpy(buf, "SW_RESET", len); break;
        case ESP_RST_PANIC:      strlcpy(buf, "PANIC", len); break;
        case ESP_RST_INT_WDT:    strlcpy(buf, "INT_WDT", len); break;
        case ESP_RST_TASK_WDT:   strlcpy(buf, "TASK_WDT", len); break;
        case ESP_RST_WDT:        strlcpy(buf, "OTHER_WDT", len); break;
        case ESP_RST_DEEPSLEEP:  strlcpy(buf, "DEEPSLEEP", len); break;
        case ESP_RST_BROWNOUT:   strlcpy(buf, "BROWNOUT", len); break;
        case ESP_RST_SDIO:       strlcpy(buf, "SDIO", len); break;
        default:                 snprintf(buf, len, "OTHER_%d", (int)reason); break;
    }
}
