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


if __name__ == "__main__":
    unittest.main()
