#include "RcPwmCapture.h"
#include "Mus4Log.h"

#if ENABLE_RC_MCPWM_CAPTURE
#include "driver/mcpwm_cap.h"
#endif

//=============================================================
// Pin mapping: channel index -> GPIO pin
//=============================================================
const int Channels[RC_CHANNEL_COUNT] = {
    CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN, CH5_PIN, CH6_PIN
};

//=============================================================
// Raw capture state (updated in ISRs)
//=============================================================
volatile uint16_t pwm_value[RC_CHANNEL_COUNT] = {0};
volatile unsigned long last_valid_time[RC_CHANNEL_COUNT] = {0};

//=============================================================
// Pulse validation (called from ISRs)
//=============================================================
static void IRAM_ATTR acceptRcPulse(int channel, uint32_t width, unsigned long now)
{
    static uint16_t candidate_pwm[RC_CHANNEL_COUNT] = {0};
    static bool candidate_pending[RC_CHANNEL_COUNT] = {false};
    static uint16_t large_change_count[RC_CHANNEL_COUNT] = {0};
    static uint16_t last_large_pwm[RC_CHANNEL_COUNT] = {0};

    if (width < RC_PWM_MIN || width > RC_PWM_MAX) return;

    uint16_t pulse = (uint16_t)width;
    uint16_t prev = pwm_value[channel];
    int diff = abs((int)pulse - (int)prev);

    // 小幅变化（≤60µs）：即时接受，覆盖正常抖动和微调，零延迟。
    // 阈值 60µs ≈ 6% 转向行程，在灵敏度和噪声抑制之间取得平衡。
    if (diff <= 60) {
        pwm_value[channel] = pulse;
        last_valid_time[channel] = now;
        candidate_pending[channel] = false;
    } else if (diff <= 200) {
        // 中幅变化（61–200µs）：需要 1 帧确认，防止单帧噪声尖峰穿透。
        // candidate_pending 标志确保只有连续两帧一致的候选值才会被接受，
        // 同时消除陈旧 candidate_pwm 残留导致的误匹配。
        if (candidate_pending[channel] && abs((int)pulse - (int)candidate_pwm[channel]) < 80) {
            pwm_value[channel] = pulse;
            last_valid_time[channel] = now;
            candidate_pending[channel] = false;
        } else {
            candidate_pwm[channel] = pulse;
            candidate_pending[channel] = true;
        }
    } else {
        // 大幅变化（>200µs）：保持原有 2 帧确认机制不变。
        if (abs((int)pulse - (int)last_large_pwm[channel]) < 100) {
            large_change_count[channel]++;
            if (large_change_count[channel] >= 2) {
                pwm_value[channel] = pulse;
                last_valid_time[channel] = now;
                large_change_count[channel] = 0;
            }
        } else {
            large_change_count[channel] = 0;
        }
        last_large_pwm[channel] = pulse;
        candidate_pending[channel] = false;
    }
}

//=============================================================
// Pin-change interrupt handler
//=============================================================
static int pin_state[RC_CHANNEL_COUNT] = {0};
static unsigned long last_edge_time[RC_CHANNEL_COUNT] = {0};
static unsigned long last_rise_time[RC_CHANNEL_COUNT] = {0};

void IRAM_ATTR handle_interrupt(int channel)
{
    unsigned long now = micros();
    if (now - last_edge_time[channel] < 100) return;
    last_edge_time[channel] = now;

    pin_state[channel] = digitalRead(Channels[channel]);
    if (pin_state[channel] == HIGH)
    {
        last_rise_time[channel] = now;
    }
    else
    {
        acceptRcPulse(channel, now - last_rise_time[channel], now);
    }
}

//=============================================================
// Per-channel ISR stubs
//=============================================================
void IRAM_ATTR CH1_interrupt() { handle_interrupt(CH_STEERING); }
void IRAM_ATTR CH2_interrupt() { handle_interrupt(CH_THROTTLE); }
void IRAM_ATTR CH3_interrupt() { handle_interrupt(CH_PARK); }
void IRAM_ATTR CH4_interrupt() { handle_interrupt(CH_MODE); }
void IRAM_ATTR CH5_interrupt() { handle_interrupt(CH_DRIFT); }
void IRAM_ATTR CH6_interrupt() { handle_interrupt(CH_DRIFT_SCALE); }

void (*isr_functions[RC_CHANNEL_COUNT])() = {
    CH1_interrupt, CH2_interrupt, CH3_interrupt,
    CH4_interrupt, CH5_interrupt, CH6_interrupt
};

//=============================================================
// Optional MCPWM capture for CH4 (mode channel)
//=============================================================
#if ENABLE_RC_MCPWM_CAPTURE
static mcpwm_cap_timer_handle_t rcMcpwmCaptureTimer = nullptr;
static mcpwm_cap_channel_handle_t rcModeCaptureChannel = nullptr;
static volatile uint32_t rcModeLastRiseTick = 0;
static volatile bool rcModeHasRiseTick = false;

static bool IRAM_ATTR onRcModeCapture(mcpwm_cap_channel_handle_t channel,
                                      const mcpwm_capture_event_data_t *edata,
                                      void *user_data)
{
    (void)channel;
    (void)user_data;
    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        rcModeLastRiseTick = edata->cap_value;
        rcModeHasRiseTick = true;
    } else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG && rcModeHasRiseTick) {
        uint32_t width = edata->cap_value - rcModeLastRiseTick;
        acceptRcPulse(CH_MODE, width, micros());
    }
    return false;
}

static bool setupRcMcpwmCaptureInternal()
{
    mcpwm_capture_timer_config_t timerConfig = {};
    timerConfig.group_id = RC_MCPWM_CAPTURE_GROUP_ID;
    timerConfig.clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT;
    timerConfig.resolution_hz = RC_MCPWM_CAPTURE_RESOLUTION_HZ;

    esp_err_t err = mcpwm_new_capture_timer(&timerConfig, &rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer init failed: %d", err);
        return false;
    }

    mcpwm_capture_channel_config_t channelConfig = {};
    channelConfig.gpio_num = CH4_PIN;
    channelConfig.prescale = 1;
    channelConfig.flags.pos_edge = true;
    channelConfig.flags.neg_edge = true;
    channelConfig.flags.pull_down = true;

    err = mcpwm_new_capture_channel(rcMcpwmCaptureTimer, &channelConfig, &rcModeCaptureChannel);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 channel init failed: %d", err);
        return false;
    }

    mcpwm_capture_event_callbacks_t callbacks = {};
    callbacks.on_cap = onRcModeCapture;
    err = mcpwm_capture_channel_register_event_callbacks(rcModeCaptureChannel, &callbacks, nullptr);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 callback init failed: %d", err);
        return false;
    }

    err = mcpwm_capture_channel_enable(rcModeCaptureChannel);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM CH4 channel enable failed: %d", err);
        return false;
    }

    err = mcpwm_capture_timer_enable(rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer enable failed: %d", err);
        return false;
    }

    err = mcpwm_capture_timer_start(rcMcpwmCaptureTimer);
    if (err != ESP_OK) {
        mus4Logf("rc", "MCPWM timer start failed: %d", err);
        return false;
    }

    mus4LogLine("rc", "MCPWM capture enabled for CH4");
    return true;
}
#endif // ENABLE_RC_MCPWM_CAPTURE

//=============================================================
// Public API
//=============================================================
void setupRcPwmCapture()
{
#if ENABLE_RC_MCPWM_CAPTURE
    bool rcMcpwmCaptureActive = setupRcMcpwmCaptureInternal();
#endif

    for (int i = 0; i < RC_CHANNEL_COUNT; i++)
    {
#if ENABLE_RC_MCPWM_CAPTURE
        if (i == CH_MODE && rcMcpwmCaptureActive) continue;
#endif
        if (Channels[i] == 26) {
            pinMode(Channels[i], INPUT_PULLDOWN);
        } else {
            pinMode(Channels[i], INPUT);
        }
        attachInterrupt(digitalPinToInterrupt(Channels[i]), isr_functions[i], CHANGE);
    }
}
