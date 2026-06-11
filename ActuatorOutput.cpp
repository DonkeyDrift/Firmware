#include "ActuatorOutput.h"
#include "SharedTypes.h"

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
