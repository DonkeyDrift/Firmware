#include "RcFilter.h"

#include "FirmwareConfig.h"
#include "Mus4Log.h"

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
