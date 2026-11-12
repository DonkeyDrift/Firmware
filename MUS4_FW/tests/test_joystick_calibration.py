"""Unit tests for the joystick calibration mapping logic.

These tests mirror the Arduino implementation in Python so they can run
without flashing hardware. Keep them in sync with JoystickCalibration.cpp.
"""

import pathlib
import re
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


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
SKETCH_INO = PROJECT_ROOT / "MUS4_FW.ino"
JOYSTICK_CAL_CPP = PROJECT_ROOT / "libraries" / "mus4_control" / "src" / "JoystickCalibration.cpp"

MOTOR_MID_DEFAULT = 7372  # ActuatorOutput.cpp 中 motor_mid_v 的编译期默认（1500µs @ 300Hz/14bit）


def load_throttle_output_limits(stored_min, stored_max, motor_mid_v):
    """Python mirror of loadThrottleOutputLimits() 的持久化值校验。

    stored_min / stored_max 传 None 表示 NVS 中无对应键。校验区间与 C++ 一致：
    Min T ∈ [4915, motor_mid_v]，Max T ∈ [motor_mid_v, 9830]，越界回退默认值。
    """
    min_duty, max_duty = 4915, 9830
    if stored_min is not None and 4915 <= stored_min <= motor_mid_v:
        min_duty = stored_min
    if stored_max is not None and motor_mid_v <= stored_max <= 9830:
        max_duty = stored_max
    return min_duty, max_duty


def setup_load_order():
    """按 MUS4_FW.ino 中调用语句出现的先后顺序提取输出配置/摇杆校准加载顺序。"""
    source = SKETCH_INO.read_text(encoding="utf-8")
    pattern = r"^[ \t]*(loadServoOutputConfig|loadMotorOutputConfig|loadJoystickCalibration)\(\);"
    return [m.group(1) for m in re.finditer(pattern, source, re.MULTILINE)]


def simulate_boot_throttle_limits(motor_mid_persisted, min_persisted, max_persisted):
    """按 setup 实际调用顺序模拟开机后的 motor_mid_v 与油门输出限幅。

    motor_mid_v 从编译期默认 7372 出发，loadMotorOutputConfig() 把它替换为
    持久化值；loadJoystickCalibration() 末尾调用 loadThrottleOutputLimits()，
    用当时的 motor_mid_v 校验持久化限幅——顺序错了限幅就会被静默回退（#196）。
    """
    motor_mid_v = MOTOR_MID_DEFAULT
    min_duty, max_duty = 4915, 9830  # JoystickCalibrationData 成员默认值
    for call in setup_load_order():
        if call == "loadMotorOutputConfig":
            motor_mid_v = motor_mid_persisted
        elif call == "loadJoystickCalibration":
            min_duty, max_duty = load_throttle_output_limits(
                min_persisted, max_persisted, motor_mid_v)
    return motor_mid_v, min_duty, max_duty


def joystick_calibration_body():
    """提取 JoystickCalibration.cpp 中 loadJoystickCalibration() 的函数体。"""
    source = JOYSTICK_CAL_CPP.read_text(encoding="utf-8")
    m = re.search(r"void loadJoystickCalibration\(\)\s*\{(.*?)^\}",
                  source, re.DOTALL | re.MULTILINE)
    assert m, "未找到 loadJoystickCalibration() 函数体"
    return m.group(1)


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


class TestPersistedThrottleLimitLoading(unittest.TestCase):
    """#196：持久化油门限幅（Min T / Max T）开机加载回归。

    loadThrottleOutputLimits() 以 motor_mid_v 为校验基准。若摇杆校准先于
    电机输出中点配置加载，持久化限幅会按编译期默认中点 7372 校验：
    mid=7000 时 Max T=7100、mid=8000 时 Min T=7500 都会被静默回退到全范围。
    """

    def test_setup_loads_output_config_before_joystick_calibration(self):
        """setup 必须先加载舵机/电机输出中点，再加载摇杆校准。"""
        order = setup_load_order()
        self.assertIn("loadJoystickCalibration", order)
        self.assertLess(order.index("loadServoOutputConfig"),
                        order.index("loadJoystickCalibration"))
        self.assertLess(order.index("loadMotorOutputConfig"),
                        order.index("loadJoystickCalibration"))

    def test_motor_mid_7000_keeps_max_7100(self):
        """mid=7000 时 Max T=7100 合法，开机后必须保持 7100 而非回退 9830。"""
        motor_mid_v, min_duty, max_duty = simulate_boot_throttle_limits(7000, None, 7100)
        self.assertEqual(motor_mid_v, 7000)
        self.assertEqual((min_duty, max_duty), (4915, 7100))

    def test_motor_mid_8000_keeps_min_7500(self):
        """mid=8000 时 Min T=7500 合法，开机后必须保持 7500 而非回退 4915。"""
        motor_mid_v, min_duty, max_duty = simulate_boot_throttle_limits(8000, 7500, None)
        self.assertEqual(motor_mid_v, 8000)
        self.assertEqual((min_duty, max_duty), (7500, 9830))


class TestLegacyMigrationLoadsThrottleLimits(unittest.TestCase):
    """#196：legacy 摇杆校准迁移分支提前 return 前必须加载油门输出限幅。"""

    def test_legacy_migration_loads_limits_before_return(self):
        """legacy 分支 saveJoystickCalibration() 后提前 return，不会走到函数
        末尾的 loadThrottleOutputLimits()；return 前必须补调一次。"""
        body = joystick_calibration_body()
        legacy_start = body.find("MUS4_PREF_STEER_MIN_KEY")
        self.assertGreater(legacy_start, -1, "未找到 legacy steering 迁移分支")
        legacy_branch = body[legacy_start:]
        return_pos = legacy_branch.find("return;")
        self.assertGreater(return_pos, -1, "legacy 迁移分支缺少提前 return")
        self.assertIn("loadThrottleOutputLimits", legacy_branch[:return_pos],
                      "legacy 迁移分支提前 return 前未加载油门输出限幅")


if __name__ == "__main__":
    unittest.main()
