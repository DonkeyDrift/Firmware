#pragma once

#define ENABLE_WIFI_CONSOLE
#ifdef ENABLE_WIFI_CONSOLE
#define ENABLE_WIFI_WEBSOCKET_TELEMETRY
#endif
// #define ENABLE_DIAGNOSTIC_COMMANDS
// #define ENABLE_BOOT_STEERING_SELF_TEST

#ifndef ENABLE_WIFI_CONSOLE
#define ENABLE_GAMEPAD_MODE
#endif

#ifdef ENABLE_WIFI_CONSOLE
#define ENABLE_WIFI_NETBIOS_DISCOVERY
#define ENABLE_WIFI_LLMNR_DISCOVERY
#endif

#define MUS4_LOG_TARGET_SERIAL 0
#define MUS4_LOG_TARGET_WEB 1
#ifndef MUS4_LOG_TARGET
#ifdef ENABLE_WIFI_CONSOLE
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_WEB
#else
#define MUS4_LOG_TARGET MUS4_LOG_TARGET_SERIAL
#endif
#endif

// RC Receiver Calibration Defaults (PWM pulse width in microseconds)
#define RC_THROTTLE_MIN 888
#define RC_THROTTLE_MID 1493
#define RC_THROTTLE_MAX 2149
#define RC_STEERING_MIN 872
#define RC_STEERING_MID 1488
#define RC_STEERING_MAX 2113

#define BUZZER_PIN 2

#define CH1_PIN 36 // Receiver PWM input CH1
#define CH2_PIN 39 // Receiver PWM input CH2
#define CH3_PIN 34 // Receiver PWM input CH3
#define CH4_PIN 26 // Receiver PWM input CH4
#define CH5_PIN 27 // Receiver PWM input CH5
#define CH6_PIN 35 // Receiver PWM input CH6

#define STEERING_PIN 23 // CH1 steering servo
#define THROTTLE_PIN 25 // CH2 throttle ESC

#define PWM_1 32 // PWM output channel 1
#define PWM_2 33 // PWM output channel 2

#define LED_PIN 5
#define NUM_LEDS 1
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define BAUD_RATE_0 115200
#define RX_1_PIN 16
#define TX_1_PIN 17
// #define RX_1_PIN 19
// #define TX_1_PIN 18      // MU02 cannot connect; use pins 16 and 17 consistently.
#define BAUD_RATE_1 115200
#define UART_SEL 12

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_SPEED 400000L

#define RC_CHANNEL_COUNT 6
#define CH_STEERING 0    // index of pwm_value[]
#define CH_THROTTLE 1    // index of pwm_value[]
#define CH_PARK 2        // index of pwm_value[]
#define CH_MODE 3        // index of pwm_value[]
#define CH_DRIFT 4       // index of pwm_value[]
#define CH_DRIFT_SCALE 5 // index of pwm_value[]

#define CAR_MODE_MANUAL 0    // 0: RC manual mode
#define CAR_MODE_SEMI_AUTO 1 // 1: Pilot steering with manual throttle
#define CAR_MODE_FULL_AUTO 2 // 2: autonomous driving mode
#define MODE_PWM_MANUAL_MAX 1250
#define MODE_PWM_FULL_AUTO_MIN 1750

#define PARK_LOCKED true     // Locked state
#define PARK_UNLOCKED false  // Unlocked state

// Park channel (CH3) hysteresis thresholds to avoid accidental park/unpark
// caused by PWM noise around the midpoint.
#define PARK_PWM_PRESS_THRESHOLD   1700  // µs: above this is considered pressed
#define PARK_PWM_RELEASE_THRESHOLD 1300  // µs: below this is considered released

#define RC_SIGNAL_TIMEOUT 1000000UL  // RC signal timeout (µs)
#define RC_PWM_MIN 800   // Minimum valid PWM (µs)
#define RC_PWM_MAX 2200  // Maximum valid PWM (µs)
#define ENABLE_RC_MCPWM_CAPTURE 0
#define RC_MCPWM_CAPTURE_RESOLUTION_HZ 1000000
#define RC_MCPWM_CAPTURE_GROUP_ID 0

#define PWM_FILTER_SIZE 5  // Sliding-window median filter size (5-7)

// Terminal control macros
#define CLEAR_SCREEN "\033[2J"         // Clear screen
#define CURSOR_HOME "\033[H"           // Move cursor to home position
#define CURSOR_UP(n) "\033[" #n "A"    // Move cursor up n rows
#define CURSOR_DOWN(n) "\033[" #n "B"  // Move cursor down n rows
#define CURSOR_RIGHT(n) "\033[" #n "C" // Move cursor right n columns
#define CURSOR_LEFT(n) "\033[" #n "D"  // Move cursor left n columns
#define SAVE_CURSOR "\033[s"           // Save cursor position
#define RESTORE_CURSOR "\033[u"        // Restore cursor position
#define HIDE_CURSOR "\033[?25l"        // Hide cursor
#define SHOW_CURSOR "\033[?25h"        // Show cursor

// Color macros
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Output control parameters
#define SENSOR_UPDATE_INTERVAL 2     // Sensor data update interval (ms) - ~500Hz
#define RC_DATA_UPDATE_INTERVAL 16   // MANUAL 模式下 T<t>S<s> 帧节奏 (ms) - ~60Hz
#define IMU_TELEMETRY_INTERVAL_MS 10 // Serial1 $IMU 帧节奏 (ms) - ~100Hz，对齐上位机 GRU W=16 ring buffer
#define MODE_PARK_HEARTBEAT_MS 1000  // Serial1 M<m>:P<p> 心跳 (ms) - 1Hz，状态变化时立即发
#define RC_FILTER_UPDATE_INTERVAL 2   // RC filter update interval (ms) - ~500Hz, balances response and stability
#define UI_UPDATE_INTERVAL 2         // UI update interval (ms) - smooth 500Hz experience

// Waveform parameters
#ifndef WAVE_WIDTH
#define WAVE_WIDTH 20                 // Waveform width (reduced for performance)
#endif
#ifndef WAVE_HEIGHT
#define WAVE_HEIGHT 6                 // Waveform height (reduced for performance)
#endif
