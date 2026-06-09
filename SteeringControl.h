#pragma once
#include <Arduino.h>

struct PIDConfig {
    float Kp = 0.8;
    float Ki = 0.05;
    float Kd = 0.2;
    float integral_limit = 50.0;
    float deadband = 2.0;
};

struct PIDState {
    float integral = 0;
    float prev_error = 0;
    float current_smooth_output = 0;
};

extern PIDConfig pid_config;
extern PIDState pid_state;
extern bool safe_mode_active;

void reset_steering_filter();
int process_steering_signal(int raw_pwm);

#ifdef ENABLE_BOOT_STEERING_SELF_TEST
void run_steering_tests();
#endif
