#include "ActuatorOutput.h"
#include "SharedTypes.h"
#include "JoystickCalibration.h"

extern ControlData car_output;

// 300Hz PWM output parameters
const int PWM_PERIOD_US = 3333;  // 300Hz period (µs)
const int PWM_MIN_V = 4915;      // 1000µs @ 300Hz
const int PWM_MAX_V = 9830;      // 2000µs @ 300Hz
extern const int MOTOR_RANGE_V = 2458; // ±500µs range
int servo_mid_v = 7372;                // 用户设定的目标中点（UI 立即显示），NVS 持久化
int motor_mid_v = 7372;                // 用户设定的目标中点（UI 立即显示），NVS 持久化
extern const int SERVO_RANGE_V = 2458; // ±500µs range
int actuator_steering_duty = 7372;     // last written steering ledc duty
int actuator_throttle_duty = 7372;     // last written throttle ledc duty

// PWM 映射实际使用的中点，仅在 steering/throttle==0 时同步 servo_mid_v/motor_mid_v，
// 避免 Set 新中点后立即叠加当前转轮偏移导致舵机/电调突变。
// 初始值 0 标记未同步，首次调用 updateActuatorOutput() 时从 servo_mid_v/motor_mid_v 复制。
static int active_servo_mid = 0;
static int active_motor_mid = 0;

void setupActuatorOutput()
{
    ledcAttachChannel(STEERING_PIN, 300, 14, CH_STEERING);
    ledcAttachChannel(THROTTLE_PIN, 300, 14, CH_THROTTLE);
}

void updateActuatorOutput()
{
    // 首次同步：从 NVS 加载的 servo_mid_v/motor_mid_v 复制到活跃中点。
    if (active_servo_mid == 0 && active_motor_mid == 0) {
        active_servo_mid = servo_mid_v;
        active_motor_mid = motor_mid_v;
    }

    // 当转轮归零时，将目标中点同步到活跃中点，平滑切换。
    if (active_servo_mid != servo_mid_v && car_output.steering == 0) {
        active_servo_mid = servo_mid_v;
    }
    if (active_motor_mid != motor_mid_v && car_output.throttle == 0) {
        active_motor_mid = motor_mid_v;
    }

    int pwm_steering = map(car_output.steering, -100, 100, active_servo_mid - SERVO_RANGE_V, active_servo_mid + SERVO_RANGE_V);
    int pwm_throttle = map(car_output.throttle, -100, 100, active_motor_mid - MOTOR_RANGE_V, active_motor_mid + MOTOR_RANGE_V);

    pwm_steering = min(max(pwm_steering, PWM_MIN_V), PWM_MAX_V);
    pwm_throttle = min(max(pwm_throttle, (int)joystick_cal.throttle_min_duty), (int)joystick_cal.throttle_max_duty);

    actuator_steering_duty = pwm_steering;
    actuator_throttle_duty = pwm_throttle;

    ledcWriteChannel(CH_STEERING, pwm_steering);
    ledcWriteChannel(CH_THROTTLE, pwm_throttle);
}
