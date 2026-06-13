#include "MUS4.h"

// This file groups related MUS4 firmware implementation sections so the
// Arduino project stays easy to browse without changing runtime behavior.

// ============================================================================
// Section: RcFilter.cpp
// ============================================================================
extern uint16_t aux_stable_pwm[];
extern uint16_t aux_candidate_pwm[];
extern uint8_t aux_candidate_count[];
extern bool aux_stable_initialized[];
extern uint16_t primary_smooth_pwm[];
extern bool primary_smooth_initialized[];

// Optimized insertion-sort median filter (O(n^2), but very fast and stable for n=5)
uint16_t medianFilter(uint16_t* buf, int size) {
    uint16_t temp[8]; // Supports up to 8 elements
    // Copy data
    for (int i = 0; i < size; i++) temp[i] = buf[i];

    // Insertion sort
    for (int i = 1; i < size; i++) {
        uint16_t key = temp[i];
        int j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j = j - 1;
        }
        temp[j + 1] = key;
    }

    // Return the median value
    return temp[size / 2];
}

bool isAuxiliaryRcChannel(int ch)
{
    return ch == CH_PARK || ch == CH_MODE || ch == CH_DRIFT || ch == CH_DRIFT_SCALE;
}

bool isPrimaryRcChannel(int ch)
{
    return ch == CH_STEERING || ch == CH_THROTTLE;
}

uint16_t smoothPrimaryPWM(int ch, uint16_t value, bool valid)
{
    if (!isPrimaryRcChannel(ch)) return value;
    if (!valid) return primary_smooth_initialized[ch] ? primary_smooth_pwm[ch] : value;
    if (!primary_smooth_initialized[ch]) {
        primary_smooth_pwm[ch] = value;
        primary_smooth_initialized[ch] = true;
        return value;
    }

    int diff = (int)value - (int)primary_smooth_pwm[ch];
    int absDiff = abs(diff);
    if (absDiff <= 6) return primary_smooth_pwm[ch];
    if (absDiff >= 80) {
        primary_smooth_pwm[ch] = value;
        return value;
    }

    primary_smooth_pwm[ch] = primary_smooth_pwm[ch] + (diff * 35) / 100;
    return primary_smooth_pwm[ch];
}

uint16_t stabilizeAuxiliaryPWM(int ch, uint16_t value, bool valid)
{
    if (!isAuxiliaryRcChannel(ch)) return value;
    if (!valid) return aux_stable_initialized[ch] ? aux_stable_pwm[ch] : value;
    if (!aux_stable_initialized[ch]) {
        aux_stable_pwm[ch] = value;
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        aux_stable_initialized[ch] = true;
        return value;
    }

    int diff = abs((int)value - (int)aux_stable_pwm[ch]);
    if (diff <= 80) {
        aux_stable_pwm[ch] = value;
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        return value;
    }

    if (abs((int)value - (int)aux_candidate_pwm[ch]) <= 80) {
        if (aux_candidate_count[ch] < 255) aux_candidate_count[ch]++;
    } else {
        aux_candidate_pwm[ch] = value;
        aux_candidate_count[ch] = 1;
    }

    if (aux_candidate_count[ch] >= 3) {
        aux_stable_pwm[ch] = value;
        aux_candidate_count[ch] = 0;
        return value;
    }

    return aux_stable_pwm[ch];
}

#ifdef ENABLE_DIAGNOSTIC_COMMANDS
bool runFilterTests()
{
    mus4LogLine("test", "Running Filter Tests...");
    bool passed = true;

    uint16_t testBuf[PWM_FILTER_SIZE];
    for(int i=0; i<PWM_FILTER_SIZE; i++) testBuf[i] = 1500;

    // 测试 1：稳态输入应保持中位数。
    uint16_t out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 1 Failed: Expected 1500, got %d", out); passed = false; }

    // 测试 2：单点 2000us 毛刺应被抑制。
    testBuf[2] = 2000;
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 2 Failed: Spike not suppressed, got %d", out); passed = false; }
    testBuf[2] = 1500;

    // 测试 3：5 点窗口中的两个离群点仍应被抑制。
    testBuf[1] = 2000;
    testBuf[2] = 2000;
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1500) { mus4Logf("test", "Filter Test 3 Failed: Double spike not suppressed, got %d", out); passed = false; }

    // 测试 4：多数样本切换后应跟随新值。
    testBuf[0] = 1600;
    testBuf[1] = 1600;
    testBuf[2] = 1600;
    out = medianFilter(testBuf, PWM_FILTER_SIZE);
    if (out != 1600) { mus4Logf("test", "Filter Test 4 Failed: Step response failed, got %d", out); passed = false; }

    primary_smooth_initialized[CH_STEERING] = false;
    uint16_t smooth = smoothPrimaryPWM(CH_STEERING, 1500, true);
    smooth = smoothPrimaryPWM(CH_STEERING, 1504, true);
    if (smooth != 1500) { mus4Logf("test", "Filter Test 5 Failed: deadband got %d", smooth); passed = false; }

    smooth = smoothPrimaryPWM(CH_STEERING, 1540, true);
    if (smooth <= 1500 || smooth >= 1540) { mus4Logf("test", "Filter Test 6 Failed: smoothing got %d", smooth); passed = false; }

    smooth = smoothPrimaryPWM(CH_STEERING, 1650, true);
    if (smooth != 1650) { mus4Logf("test", "Filter Test 7 Failed: passthrough got %d", smooth); passed = false; }
    primary_smooth_initialized[CH_STEERING] = false;
    primary_smooth_pwm[CH_STEERING] = 0;

    if (passed) mus4LogLine("test", "Filter Tests Passed!");
    return passed;
}
#endif

// ============================================================================
// Section: RcPwmCapture.cpp
// ============================================================================
#if ENABLE_RC_MCPWM_CAPTURE
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
    static uint16_t large_change_count[RC_CHANNEL_COUNT] = {0};
    static uint16_t last_large_pwm[RC_CHANNEL_COUNT] = {0};

    if (width < RC_PWM_MIN || width > RC_PWM_MAX) return;

    uint16_t pulse = (uint16_t)width;
    uint16_t prev = pwm_value[channel];
    int diff = abs((int)pulse - (int)prev);

    if (diff <= 120) {
        pwm_value[channel] = pulse;
        last_valid_time[channel] = now;
    } else if (diff <= 200) {
        if (abs((int)pulse - (int)candidate_pwm[channel]) < 80) {
            pwm_value[channel] = pulse;
            last_valid_time[channel] = now;
        }
        candidate_pwm[channel] = pulse;
    } else {
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

// ============================================================================
// Section: SteeringControl.cpp
// ============================================================================
// --- Steering Signal Processing Constants & Globals ---
const int PWM_VALID_MIN = 800; // Increased from 500 to reject noise
const int PWM_VALID_MAX = 2200; // Decreased from 2500 to reject noise
const int MA_WINDOW_SIZE = 10;
const int MAX_ERROR_COUNT = 3;

PIDConfig pid_config;
PIDState pid_state;

int steering_history[MA_WINDOW_SIZE] = {0};
int steering_index = 0;
int last_valid_steering_pwm = 1488; // Default to center
int steering_error_count = 0;
int valid_signal_count = 0; // New: Counter for valid signals to exit safe mode
bool safe_mode_active = false;
bool is_history_initialized = false;

void reset_steering_filter() {
    for (int i = 0; i < MA_WINDOW_SIZE; i++) {
        steering_history[i] = 1488;
    }
    steering_index = 0;
    last_valid_steering_pwm = 1488;
    steering_error_count = 0;
    valid_signal_count = 0;
    safe_mode_active = false;
    is_history_initialized = true;

    // Reset PID State
    pid_state.integral = 0;
    pid_state.prev_error = 0;
    pid_state.current_smooth_output = 0;
}

int process_steering_signal(int raw_pwm) {
    // 0. Initialize history if needed
    if (!is_history_initialized) {
        reset_steering_filter();
    }

    // 1. Input Validation (Data Acquisition Layer)
    int current_pwm = raw_pwm;
    bool is_signal_valid = true;

    // Check range
    if (raw_pwm < PWM_VALID_MIN || raw_pwm > PWM_VALID_MAX) {
        // Invalid signal: use last valid value
        current_pwm = last_valid_steering_pwm;
        is_signal_valid = false;
    }
    // Check slew rate (spike detection)
    // Reject if change > 800us in single frame (impossible for human input)
    // unless it persists (handled by consecutive valid checks, but for now simple rejection)
    else if (abs(raw_pwm - last_valid_steering_pwm) > 800) {
        // Treat as noise spike
        current_pwm = last_valid_steering_pwm;
        is_signal_valid = false;
        // Serial.println("Warn: Steering Signal Spike Detected!");
    }
    else {
        last_valid_steering_pwm = current_pwm;
    }

    // 2. Smoothing (Moving Average) - Pre-filter
    steering_history[steering_index] = current_pwm;
    steering_index = (steering_index + 1) % MA_WINDOW_SIZE;

    long sum = 0;
    for (int i = 0; i < MA_WINDOW_SIZE; i++) {
        sum += steering_history[i];
    }
    int filtered_pwm = sum / MA_WINDOW_SIZE;

    // 3. Mapping to Control Range (-100 to 100)
    // Target steering based on filtered PWM
    int16_t cal_mid = steer_cal_enabled ? steer_cal.mid_pwm : RC_STEERING_MID;
    int16_t cal_min = steer_cal_enabled ? steer_cal.min_pwm : RC_STEERING_MIN;
    int16_t cal_max = steer_cal_enabled ? steer_cal.max_pwm : RC_STEERING_MAX;
    float target_steering;
    if (filtered_pwm < cal_mid) {
        target_steering = map(filtered_pwm - cal_mid, cal_min - cal_mid, 0, -100, 0);
    } else {
        target_steering = map(filtered_pwm - cal_mid, 0, cal_max - cal_mid, 0, 100);
    }

    // 4. PID Calculation
    float error = target_steering - pid_state.current_smooth_output;

    // Deadband check
    if (abs(error) < pid_config.deadband) {
        error = 0;
    }

    // Integral term
    pid_state.integral += error;
    pid_state.integral = constrain(pid_state.integral, -pid_config.integral_limit, pid_config.integral_limit);

    // Derivative term
    float derivative = error - pid_state.prev_error;

    // Calculate output change
    float output_change = (pid_config.Kp * error) + (pid_config.Ki * pid_state.integral) + (pid_config.Kd * derivative);

    // Update state
    pid_state.prev_error = error;
    pid_state.current_smooth_output += output_change;

    // 5. Post-Clamping
    int final_steering = constrain((int)pid_state.current_smooth_output, -100, 100);

    // 6. Fault Detection & Safety Mode Logic
    // Condition A: Sensor out of range (checked in step 1) or excessive value
    // Note: Since we clamp final_steering, we check the mapped target or raw signal validity

    if (!is_signal_valid || abs(target_steering) > 120) { // Allow some margin over 100 before error
        steering_error_count++;
        valid_signal_count = 0; // Reset recovery counter

        if (steering_error_count >= MAX_ERROR_COUNT) {
            if (!safe_mode_active) {
                safe_mode_active = true;
                mus4LogLine("steering", "ALARM: Steering Sensor Fault! Safe Mode Activated.");
            }
        }
    } else {
        // Signal is valid
        steering_error_count = 0; // Reset error counter

        if (safe_mode_active) {
            // Recovery logic
            valid_signal_count++;
            if (valid_signal_count > 50) { // Approx 1 second @ 50Hz (assuming loop speed)
                safe_mode_active = false;
                valid_signal_count = 0;
                mus4LogLine("steering", "INFO: Steering Signal Recovered. Exiting Safe Mode.");

                // Soft reset PID output to current target to avoid jump
                pid_state.current_smooth_output = target_steering;
            }
        }
    }

    // Override if safe mode
    if (safe_mode_active) {
        final_steering = 0; // Center steering
        pid_state.current_smooth_output = 0; // Reset PID output
        pid_state.integral = 0; // Reset integral
    }

    return final_steering;
}

#ifdef ENABLE_BOOT_STEERING_SELF_TEST
void run_steering_tests() {
    mus4LogLine("test", "--- Starting Steering Signal Processing Unit Tests (PID Enabled) ---");

    // Test 1: Normal Value (PID Convergence)
    reset_steering_filter();
    int res = 0;
    // Simulate convergence
    for(int i=0; i<20; i++) {
        res = process_steering_signal(1488);
    }
    mus4Logf("test", "Test 1 (Normal 1488 -> 0): Output=%d, Pass=%d", res, res == 0);

    // Test 2: Boundary Values
    reset_steering_filter();
    // Fill buffer to avoid smoothing delay effect for test
    for(int i=0; i<10; i++) process_steering_signal(872);
    // Run PID loop to converge
    for(int i=0; i<20; i++) res = process_steering_signal(872);
    mus4Logf("test", "Test 2A (Min 872 -> -100): Output=%d, Pass=%d", res, res == -100);

    reset_steering_filter();
    for(int i=0; i<10; i++) process_steering_signal(2113);
    for(int i=0; i<20; i++) res = process_steering_signal(2113);
    mus4Logf("test", "Test 2B (Max 2113 -> 100): Output=%d, Pass=%d", res, res == 100);

    // Test 3: Noise Injection (Should be ignored or dampened)
    reset_steering_filter();
    // Converge to center
    for(int i=0; i<20; i++) process_steering_signal(1488);

    // Inject single frame noise (0 is invalid PWM, so it uses last valid 1488)
    int noise_res = process_steering_signal(0);
    mus4Logf("test", "Test 3 (Invalid Input 0 -> Hold Last): Output=%d, Pass=%d", noise_res, noise_res == 0);

    // Test 4: Hard Clamping
    reset_steering_filter();
    // Inject value that maps to > 100 but is valid PWM (e.g. 2200)
    for(int i=0; i<30; i++) res = process_steering_signal(2200);
    mus4Logf("test", "Test 4 (Clamp 2200 -> 100): Output=%d, Pass=%d", res, res == 100);

    // Test 5: Safety Mode Activation
    reset_steering_filter();
    // Trigger error.
    // Since we have a 10-point moving average, we need enough samples for the average to cross the threshold.
    // Target threshold > 120 corresponds to filtered_pwm > approx 2237.
    // Input 2300.
    for(int i=0; i<15; i++) {
        process_steering_signal(2300);
    }
    mus4Logf("test", "Test 5 (Safety Mode Activation): Active=%d, Pass=%d", safe_mode_active, safe_mode_active == true);

    // Test 6: Safety Mode Recovery
    // Continue from Test 5, safe_mode_active is true.
    // Feed valid signals. We need > 50 valid signals.
    for(int i=0; i<50; i++) {
        process_steering_signal(1488);
    }
    // Should still be active (count = 50)
    bool still_active = safe_mode_active;

    // One more
    process_steering_signal(1488);
    bool recovered = !safe_mode_active;

    mus4Logf("test", "Test 6 (Safety Mode Recovery): Still Active at 50=%d, Recovered at 51=%d, Pass=%d",
                  still_active, recovered, still_active && recovered);

    mus4LogLine("test", "--- End Tests ---");
    reset_steering_filter(); // Reset for actual operation
}
#endif

// ============================================================================
// Section: DriftAssist.cpp
// ============================================================================
// --- 漂移辅助常量与运行状态 ---
#define DRIFT_ASSIST_ENABLED     1        // 编译期开启漂移辅助
#define DRIFT_ASSIST_GAIN        25.0f    // 反打增益：gyroZ rad/s -> ±100 补偿
#define DRIFT_ASSIST_THRESHOLD   1.2f     // 低于该角速度阈值时不介入
#define DRIFT_ASSIST_MAX_COMP    70       // 限制最大补偿角度，避免过度反打
#define DRIFT_ASSIST_SMOOTH      0.25f    // 一阶平滑系数，降低输出抖动
#define DRIFT_ASSIST_DECAY       0.85f    // 未触发漂移时的补偿衰减系数

bool drift_assist_enabled = false;   // 用户是否已启用辅助
bool drift_assist_active = false;    // 辅助当前是否正在介入
float drift_compensation = 0.0f;     // 当前平滑后的补偿量
float gyro_z_filtered = 0.0f;        // 滤波后的 gyroZ
float drift_assist_scale = 1.0f;     // CH6 漂移辅助强度比例
// ------------------------------------------------------

extern uint16_t pwm_filtered[];
extern SensorData mpu6050Data;
extern ControlData car_output;

void update_drift_assist_control(bool driftValid, bool driftScaleValid)
{
    if (driftScaleValid) {
        uint16_t scalePwm = constrain(pwm_filtered[CH_DRIFT_SCALE], 1000, 2000);
        drift_assist_scale = (scalePwm - 1000) / 500.0f;
    } else {
        drift_assist_scale = 1.0f;
    }

    bool enabled = driftValid && pwm_filtered[CH_DRIFT] > 1500;
    if (!enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        gyro_z_filtered = 0.0f;
    }
    drift_assist_enabled = enabled;
}

// --- 漂移辅助逻辑 ---
// 输入：driver_steering 为驾驶员原始转向输入（-100~100）。
// 输出：叠加漂移补偿后的最终转向值（-100~100）。
int apply_drift_assist(int driver_steering) {
#if DRIFT_ASSIST_ENABLED
    // 仅在手动模式且漂移辅助已启用时介入。
    if (car_output.mode != CAR_MODE_MANUAL || !drift_assist_enabled) {
        drift_assist_active = false;
        drift_compensation = 0.0f;
        return driver_steering;
    }

    // 1. Apply a first-order low-pass filter to gyroZ to remove sensor noise
    gyro_z_filtered = gyro_z_filtered * (1.0f - DRIFT_ASSIST_SMOOTH) +
                      mpu6050Data.gyroZ * DRIFT_ASSIST_SMOOTH;

    // 2. Determine whether drift is triggered
    float abs_gyro = fabs(gyro_z_filtered);
    if (abs_gyro > DRIFT_ASSIST_THRESHOLD) {
        // 3. Calculate counter-steer compensation. A negative sign would invert direction
        // (clockwise slide -> negative gyro -> positive compensation -> steer right?),
        // but the physical direction must be aligned first.
        // User definition: clockwise rear slide gives negative gyroZ -> counter-steer left is needed (<1439 -> negative value).
        // Therefore, negative gyroZ should produce negative compensation.
        // Formula: comp = gyroZ * GAIN (keep the same sign).
        // Compensation direction:
        // - gyroZ > 0: vehicle is rotating counter-clockwise.
        // - gyroZ < 0: vehicle is rotating clockwise.
        // User calibration:
        // Steering PWM center is 1439.
        // PWM < 1439 means steering left (negative normalized steering).
        // PWM > 1439 means steering right (positive normalized steering).
        // Clockwise rear slide (rear swings right / oversteers right) -> counter-steer left -> add a negative steering value.
        // At this moment gyroZ is negative -> compensation should also be negative.
        // Therefore compensation = gyroZ * GAIN.
        // Implement this logic first; adjust the sign during real-vehicle tuning if needed.
        float raw_comp = gyro_z_filtered * DRIFT_ASSIST_GAIN * drift_assist_scale;

        // 4. Clamp compensation
        float effectiveMaxComp = min(DRIFT_ASSIST_MAX_COMP * drift_assist_scale, 100.0f);
        raw_comp = constrain(raw_comp, -effectiveMaxComp, effectiveMaxComp);

        // 5. Smooth compensation output
        drift_compensation = drift_compensation * (1.0f - DRIFT_ASSIST_SMOOTH) +
                             raw_comp * DRIFT_ASSIST_SMOOTH;

        drift_assist_active = true;
    } else {
        // Below threshold; gradually decay compensation to 0
        drift_compensation *= DRIFT_ASSIST_DECAY;
        if (fabs(drift_compensation) < 0.5f) {
            drift_compensation = 0.0f;
            drift_assist_active = false;
        } else {
            drift_assist_active = true;
        }
    }

    // 6. Add compensation to the raw driver input and clamp the result
    int final_steering = driver_steering + (int)drift_compensation;
    final_steering = constrain(final_steering, -100, 100);

    return final_steering;
#else
    drift_assist_active = false;
    drift_compensation = 0.0f;
    return driver_steering;
#endif
}

// ============================================================================
// Section: ControlMixer.cpp
// ============================================================================
#ifdef ENABLE_GAMEPAD_MODE
#endif

extern ControlData rc_data;
extern ControlData pilot_data;
extern ControlData car_output;
extern uint16_t pwm_filtered[];
extern bool steer_cal_enabled;
extern bool toggleActive;
extern Buzzer buzzer;

static int lastCarMode = -1;

void mode_change(bool modeValid)
{
    if (!modeValid) {
        return;
    }

    rc_data.mode = pwm_filtered[CH_MODE];
    if (rc_data.mode <= MODE_PWM_MANUAL_MAX)
    {
        car_output.mode = CAR_MODE_MANUAL; // 0: RC manual mode
    }
    else if (rc_data.mode >= MODE_PWM_FULL_AUTO_MIN)
    {
        car_output.mode = CAR_MODE_FULL_AUTO; // 2: autonomous driving mode
    }
    else
    {
        car_output.mode = CAR_MODE_SEMI_AUTO; // 1: Pilot steering with manual throttle
    }

    if (car_output.mode != lastCarMode)
    {
        buzzer.playModeSound(car_output.mode);
        lastCarMode = car_output.mode;
    }
}

static int carOutputModeLast = -1;

void updateControlOutput()
{
    if (car_output.mode == CAR_MODE_FULL_AUTO)
    {
        // Controlled by Pilot
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_FULL_AUTO || !toggleActive)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
                carOutputModeLast = CAR_MODE_FULL_AUTO;
            }
            if (!toggleActive)
            {
                setLEDToggle(CRGB::Blue, CRGB::Red);
            }
        }
        else
        {
            setLEDColor(CRGB::Blue); // set LED to Red
            car_output.throttle = pilot_data.throttle;
        }
        car_output.steering = pilot_data.steering;

        #ifdef ENABLE_GAMEPAD_MODE
            sendGamepadPacket();
        #endif
    }
    else if (car_output.mode == CAR_MODE_SEMI_AUTO)
    {
        // Controlled by both RC and Pilot
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_SEMI_AUTO || !toggleActive)
            {
                setLEDToggle(CRGB::Yellow, CRGB::Red);
                carOutputModeLast = CAR_MODE_SEMI_AUTO;
            }
        }
        else
        {
            setLEDColor(CRGB::Yellow); // set LED to blue
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        car_output.steering = pilot_data.steering;
    }
    else
    {
        // Controlled by RC Controller (car_output.mode = CAR_MODE_MANUAL)
        if (car_output.park == 1)
        {
            if (carOutputModeLast != CAR_MODE_MANUAL || !toggleActive)
            {
                setLEDToggle(CRGB::Green, CRGB::Red);
                carOutputModeLast = CAR_MODE_MANUAL;
            }
        }
        else
        {
            setLEDColor(CRGB::Green); // set LED to blue

            // RC => CAR
            car_output.throttle = map(rc_data.throttle, RC_THROTTLE_MIN, RC_THROTTLE_MAX, -100, 100);
        }
        if (steer_cal_enabled) {
            car_output.steering = mapSteeringCalibrated(rc_data.steering);
        } else {
            car_output.steering = map(rc_data.steering, RC_STEERING_MIN, RC_STEERING_MAX, -100, 100);
        }
        // Drift Assist: add counter-steer compensation only in manual mode
        car_output.steering = apply_drift_assist(car_output.steering);
    }
}

// ============================================================================
// Section: SafetyState.cpp
// ============================================================================
extern ControlData rc_data;
extern ControlData car_output;
extern uint16_t pwm_filtered[];
extern Buzzer buzzer;

// Emergency Stop FSM state
EmergencyStopState emergencyStopState = EST_IDLE;
unsigned long emergencyStopStartTime = 0;
const unsigned long EMERGENCY_STOP_READY_DURATION = 500;  // Brake preparation time: 500ms
const unsigned long EMERGENCY_STOP_BRAKE_DURATION = 1500; // Braking duration: 1500ms

// Park Control state
unsigned long parkBtnPressStartTime = 0;
bool parkBtnPressed = false;
bool parkActionTaken = false;
const unsigned long PARK_UNLOCK_HOLD_TIME = 1000; // 1s to Unlock
const unsigned long PARK_LOCK_HOLD_TIME = 500;    // 0.5s to Lock

void emergencyStop()
{
    // If the park signal has been released, reset the state machine
    if (car_output.park == 0 && emergencyStopState == EST_DONE)
    {
        emergencyStopState = EST_IDLE;
        mus4LogLine("tui", "Emergency Stop FSM reset: Park unlocked");
        return;
    }

    switch (emergencyStopState)
    {
    case EST_IDLE:
        if (car_output.throttle > 0)
        {
            mus4LogLine("tui", "Start Emergency stop");
            car_output.throttle = 15;
            emergencyStopState = EST_READY;
            emergencyStopStartTime = millis();
        }
        else
        {
            emergencyStopState = EST_DONE;
        }
        break;

    case EST_READY:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_READY_DURATION)
        {
            car_output.throttle = -100;
            emergencyStopState = EST_BRAKING;
            emergencyStopStartTime = millis();
            mus4LogLine("tui", "Emergency STOP ready");
        }
        break;

    case EST_BRAKING:
        if (millis() - emergencyStopStartTime >= EMERGENCY_STOP_BRAKE_DURATION)
        {
            emergencyStopState = EST_DONE;
            mus4LogLine("tui", "Emergency STOP done");
        }
        break;

    case EST_DONE:
        // Braking is complete; reset throttle to zero
        car_output.throttle = 0;
        break;
    }
}

void park_change()
{
    // PWM > 1500 considered Pressed (Button value 2000)
    // PWM < 1500 considered Released (Button value 1000)
    bool isPressed = (pwm_filtered[CH_PARK] > 1500);

    if (isPressed)
    {
        if (!parkBtnPressed)
        {
            // Rising Edge: Start Timer
            parkBtnPressed = true;
            parkBtnPressStartTime = millis();
            parkActionTaken = false;
        }
        else
        {
            // Button Held
            if (!parkActionTaken)
            {
                unsigned long duration = millis() - parkBtnPressStartTime;

                if (rc_data.park)
                { // Currently Locked (Park Mode)
                    // Unlock Logic: Hold for 1s
                    if (duration >= PARK_UNLOCK_HOLD_TIME)
                    {
                        rc_data.park = false; // Unlock
                        emergencyStopState = EST_IDLE; // Reset Emergency Stop FSM
                        parkActionTaken = true;
                        mus4LogLine("tui", "System Unlocked: Park Mode Exited");
                        buzzer.playParkUnlockSound();
                    }
                }
                else
                { // Currently Unlocked (Drive Mode)
                    // Lock Logic: Hold for 0.5s
                    if (duration >= PARK_LOCK_HOLD_TIME)
                    {
                        rc_data.park = true; // Lock
                        parkActionTaken = true;
                        mus4LogLine("tui", "System Locked: Park Mode Entered");
                        buzzer.playParkLockSound();
                    }
                }
            }
        }
    }
    else
    {
        // Button Released
        parkBtnPressed = false;
        parkActionTaken = false;
    }

    car_output.park = rc_data.park;
}

// ============================================================================
// Section: ActuatorOutput.cpp
// ============================================================================
extern ControlData car_output;

// 300Hz PWM output parameters
const int PWM_PERIOD_US = 3333;  // 300Hz period (µs)
const int PWM_MIN_V = 4915;      // 1000µs @ 300Hz
const int PWM_MAX_V = 9830;      // 2000µs @ 300Hz
const int MOTOR_MID_V = 7372;    // 1500µs @ 300Hz
const int MOTOR_RANGE_V = 2458; // ±500µs range
extern const int SERVO_MID_V = 7372;    // 1500µs @ 300Hz
extern const int SERVO_RANGE_V = 2458; // ±500µs range

void setupActuatorOutput()
{
    ledcAttachChannel(STEERING_PIN, 300, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 300, 14, CH_THROTTLE);
}

void updateActuatorOutput()
{
    int pwm_steering = map(car_output.steering, -100, 100, SERVO_MID_V - SERVO_RANGE_V, SERVO_MID_V + SERVO_RANGE_V);
    int pwm_throttle = map(car_output.throttle, -100, 100, MOTOR_MID_V - MOTOR_RANGE_V, MOTOR_MID_V + MOTOR_RANGE_V);

    pwm_steering = min(max(pwm_steering, PWM_MIN_V), PWM_MAX_V);
    pwm_throttle = min(max(pwm_throttle, PWM_MIN_V), PWM_MAX_V);

    ledcWriteChannel(CH_STEERING, pwm_steering);
    ledcWriteChannel(CH_THROTTLE, pwm_throttle);
}

