#include "RcFilter.h"

#include "FirmwareConfig.h"

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
