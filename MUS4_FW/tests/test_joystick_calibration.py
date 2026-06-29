"""Unit tests for the joystick calibration mapping logic.

These tests mirror the Arduino implementation in Python so they can run
without flashing hardware. Keep them in sync with JoystickCalibration.cpp.
"""

import pathlib
import sys
import unittest


def map_joystick_axis(pwm, cal, enabled, default_min, default_mid, default_max):
    """Python mirror of mapJoystickAxis()."""
    if not enabled:
        # 非校准路径同样使用三段式映射，以 default_mid 作为物理中点
        if pwm < default_mid:
            v = int((pwm - default_min) * 100 / (default_mid - default_min)) - 100
            return max(-100, min(0, v))
        v = int((pwm - default_mid) * 100 / (default_max - default_mid))
        return max(0, min(100, v))

    if pwm < cal["mid_pwm"]:
        v = int((pwm - cal["min_pwm"]) * 100 / (cal["mid_pwm"] - cal["min_pwm"])) - 100
        return max(-100, min(0, v))
    v = int((pwm - cal["mid_pwm"]) * 100 / (cal["max_pwm"] - cal["mid_pwm"]))
    return max(0, min(100, v))


MOTOR_RANGE_V = 2458   # ±500µs @ 300Hz/14bit
PWM_MIN_V = 4915       # 1000µs
PWM_MAX_V = 9830       # 2000µs


def throttle_duty_from_mapped(mapped, mid_duty=7372):
    """将 -100..100 映射值转换为 PWM duty（镜像 ActuatorOutput 逻辑）。"""
    duty = int(mapped * MOTOR_RANGE_V / 100 + mid_duty)
    return max(PWM_MIN_V, min(PWM_MAX_V, duty))


def map_joystick_axis_constrained(pwm, cal, enabled, default_min, default_mid, default_max,
                                  min_duty=4915, max_duty=9830, mid_duty=7372):
    """Python mirror of mapJoystickAxis() + ActuatorOutput PWM duty clamping.

    1. mapJoystickAxis() → mapped (-100..100)
    2. mapped → PWM duty（actuator 转换）
    3. constrain(duty, min_duty, max_duty) — Min T/Max T 限幅
    返回 (mapped, clamped_duty) 元组
    """
    mapped = map_joystick_axis(pwm, cal, enabled, default_min, default_mid, default_max)
    duty = int(mapped * MOTOR_RANGE_V / 100 + mid_duty)
    duty = max(min_duty, min(max_duty, duty))
    duty = max(PWM_MIN_V, min(PWM_MAX_V, duty))
    return mapped, duty


def validate_axis(axis):
    return (
        axis["min_pwm"] < axis["mid_pwm"] < axis["max_pwm"]
        and (axis["mid_pwm"] - axis["min_pwm"]) > 100
        and (axis["max_pwm"] - axis["mid_pwm"]) > 100
        and axis["min_pwm"] >= 800
        and axis["max_pwm"] <= 2200
    )


class TestJoystickCalibrationMapping(unittest.TestCase):
    def test_center_returns_zero(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(1500, cal, True, 1000, 1500, 2000), 0)

    def test_min_returns_negative_100(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(1000, cal, True, 1000, 1500, 2000), -100)

    def test_max_returns_100(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(2000, cal, True, 1000, 1500, 2000), 100)

    def test_below_min_clamps(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(500, cal, True, 1000, 1500, 2000), -100)

    def test_above_max_clamps(self):
        cal = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}
        self.assertEqual(map_joystick_axis(2500, cal, True, 1000, 1500, 2000), 100)

    def test_disabled_uses_defaults(self):
        cal = {"min_pwm": 900, "mid_pwm": 1400, "max_pwm": 1900}
        self.assertEqual(map_joystick_axis(1500, cal, False, 1000, 1500, 2000), 0)
        self.assertEqual(map_joystick_axis(1000, cal, False, 1000, 1500, 2000), -100)
        self.assertEqual(map_joystick_axis(2000, cal, False, 1000, 1500, 2000), 100)

    def test_validation_accepts_reasonable_range(self):
        self.assertTrue(validate_axis({"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}))

    def test_validation_rejects_min_equal_mid(self):
        self.assertFalse(validate_axis({"min_pwm": 1500, "mid_pwm": 1500, "max_pwm": 2000}))

    def test_validation_rejects_too_narrow_range(self):
        self.assertFalse(validate_axis({"min_pwm": 1490, "mid_pwm": 1500, "max_pwm": 1510}))

    def test_validation_rejects_out_of_pwm_bounds(self):
        self.assertFalse(validate_axis({"min_pwm": 700, "mid_pwm": 1500, "max_pwm": 2000}))
        self.assertFalse(validate_axis({"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2300}))


class TestThrottleOutputClamping(unittest.TestCase):
    """油门输出限幅（Min T / Max T）的单元测试。

    验证 mapJoystickAxis() 三段式映射后在 ActuatorOutput 层施加
    PWM duty 限幅的行为。限幅值与 Mid T 使用相同单位（raw PWM duty）。
    """

    CAL = {"min_pwm": 1000, "mid_pwm": 1500, "max_pwm": 2000}

    def test_default_full_range(self):
        """默认限幅 4915-9830（全范围），duty 不受额外限制。"""
        _, duty = map_joystick_axis_constrained(1000, self.CAL, True, 1000, 1500, 2000)
        self.assertEqual(duty, 4915)  # -100 → 4915 (1000µs)
        _, duty = map_joystick_axis_constrained(1500, self.CAL, True, 1000, 1500, 2000)
        self.assertEqual(duty, 7372)  # 0 → mid_duty
        _, duty = map_joystick_axis_constrained(2000, self.CAL, True, 1000, 1500, 2000)
        self.assertEqual(duty, 9830)  # 100 → 9830 (2000µs)

    def test_min_duty_clamps_lower(self):
        """Min T=6143（对应 mapped=-50），duty 不低于 6143。"""
        _, duty = map_joystick_axis_constrained(
            1000, self.CAL, True, 1000, 1500, 2000, min_duty=6143)
        self.assertEqual(duty, 6143)  # mapped=-100 → duty=4915，clamped 到 6143
        _, duty = map_joystick_axis_constrained(
            1500, self.CAL, True, 1000, 1500, 2000, min_duty=6143)
        self.assertEqual(duty, 7372)  # mapped=0 → duty=7372，未受限
        _, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, min_duty=6143)
        self.assertEqual(duty, 9830)  # mapped=100 → duty=9830，未受限

    def test_max_duty_clamps_upper(self):
        """Max T=8601（对应 mapped=50），duty 不高于 8601。"""
        _, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, max_duty=8601)
        self.assertEqual(duty, 8601)  # mapped=100 → duty=9830，clamped 到 8601
        _, duty = map_joystick_axis_constrained(
            1500, self.CAL, True, 1000, 1500, 2000, max_duty=8601)
        self.assertEqual(duty, 7372)  # mapped=0 → duty=7372，未受限
        _, duty = map_joystick_axis_constrained(
            1000, self.CAL, True, 1000, 1500, 2000, max_duty=8601)
        self.assertEqual(duty, 4915)  # mapped=-100 → duty=4915，未受限

    def test_both_clamp(self):
        """Min T=6143, Max T=8601，duty 限制在 [6143, 8601]。"""
        _, duty = map_joystick_axis_constrained(
            1000, self.CAL, True, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 6143)
        _, duty = map_joystick_axis_constrained(
            1500, self.CAL, True, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 7372)  # 中点未受限
        _, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 8601)

    def test_min_above_mid_duty(self):
        """Min T 设在中点以上（如 8000），中点输出也被 clamp 到 8000。"""
        _, duty = map_joystick_axis_constrained(
            1000, self.CAL, True, 1000, 1500, 2000, min_duty=8000)
        self.assertEqual(duty, 8000)  # 全部 clamped 到 >= 8000
        _, duty = map_joystick_axis_constrained(
            1500, self.CAL, True, 1000, 1500, 2000, min_duty=8000)
        self.assertEqual(duty, 8000)
        _, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, min_duty=8000)
        self.assertEqual(duty, 9830)  # 最大未受限

    def test_mapped_not_affected_by_clamping(self):
        """限幅只影响 duty，不影响 mapped 值（car_output.throttle 不变）。"""
        mapped, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, max_duty=8601)
        self.assertEqual(mapped, 100)  # mapped 始终是 100
        self.assertEqual(duty, 8601)   # duty 被限制

    def test_invalid_min_rejected_by_cpp(self):
        """Min T 超出 [4915, motor_mid_v] 在 C++ 侧被拒绝保存。
        本测试验证 duty 超出范围时仍受绝对上下限保护。"""
        _, duty = map_joystick_axis_constrained(
            1000, self.CAL, True, 1000, 1500, 2000, min_duty=4000)
        self.assertEqual(duty, 4915)  # clamped by PWM_MIN_V

    def test_invalid_max_rejected_by_cpp(self):
        """Max T 超出 [motor_mid_v, 9830] 在 C++ 侧被拒绝保存。"""
        _, duty = map_joystick_axis_constrained(
            2000, self.CAL, True, 1000, 1500, 2000, max_duty=10000)
        self.assertEqual(duty, 9830)  # clamped by PWM_MAX_V

    def test_disabled_calibration_still_clamped(self):
        """校准未启用时，duty 限幅仍然生效（限幅独立于校准 enable 标志）。"""
        cal = {"min_pwm": 900, "mid_pwm": 1400, "max_pwm": 1900}
        _, duty = map_joystick_axis_constrained(
            1500, cal, False, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 7372)  # mapped=0 → duty=7372
        _, duty = map_joystick_axis_constrained(
            1000, cal, False, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 6143)  # clamped by min
        _, duty = map_joystick_axis_constrained(
            2000, cal, False, 1000, 1500, 2000, min_duty=6143, max_duty=8601)
        self.assertEqual(duty, 8601)  # clamped by max


if __name__ == "__main__":
    unittest.main()
