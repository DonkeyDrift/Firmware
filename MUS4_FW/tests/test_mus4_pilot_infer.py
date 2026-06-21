import importlib.util
import math
import pathlib
import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "tools" / "mus4_pilot_infer.py"
SPEC = importlib.util.spec_from_file_location("mus4_pilot_infer", MODULE_PATH)
PILOT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PILOT)


class TestMus4PilotInfer(unittest.TestCase):
    def sample_latest(self, **overrides):
        latest = {
            "seq": 10,
            "dt": 22,
            "gz": 0.1,
            "gzf": 0.2,
            "mode": 2,
            "park": 0,
            "ch1": 1111,
            "ch2": 1777,
            "ch3": 1000,
            "ch4": 2000,
            "ch5": 2000,
            "ch6": 1372,
            "rct": 1777,
            "rcs": 1111,
            "thr": 44,
            "str": -33,
            "pt": 0,
            "ps": 0,
            "dc": 1.5,
            "de": 1,
            "da": 0,
            "vol": 11.2,
        }
        latest.update(overrides)
        return latest

    def standardization(self):
        return {
            "feature_columns": ["dt", "gz", "gzf", "mode", "park", "ch3", "ch4", "ch5", "ch6", "pt", "ps", "dc", "de", "da", "vol"],
            "mean": [0.0] * 15,
            "std": [1.0] * 15,
            "window_size": 3,
        }

    def test_rejects_leakage_feature_columns(self):
        stats = self.standardization()
        stats["feature_columns"] = ["dt", "ch1", "vol"]

        with self.assertRaises(ValueError):
            PILOT.validate_standardization(stats)

    def test_build_feature_vector_uses_standardization_order(self):
        latest = self.sample_latest()
        stats = self.standardization()

        vector = PILOT.build_feature_vector(latest, stats["feature_columns"])

        self.assertEqual(vector[0], 22.0)
        self.assertEqual(vector[1], 0.1)
        self.assertEqual(vector[-1], 11.2)
        self.assertNotIn(1111.0, vector)
        self.assertNotIn(1777.0, vector)

    def test_standardization_dimension_mismatch_is_rejected(self):
        stats = self.standardization()
        stats["mean"] = [0.0]

        with self.assertRaises(ValueError):
            PILOT.validate_standardization(stats)

    def test_safety_gate_blocks_manual_mode(self):
        limits = PILOT.ControlLimits(max_throttle=20, max_steering=30, max_delta_throttle=10, max_delta_steering=10)
        state = PILOT.ControlState()

        command = PILOT.apply_safety_gate((15.0, 20.0), self.sample_latest(mode=0), limits, state)

        self.assertEqual((command.throttle, command.steering), (0, 0))
        self.assertEqual(command.reason, "manual_mode")

    def test_safety_gate_blocks_park_locked(self):
        limits = PILOT.ControlLimits(max_throttle=20, max_steering=30, max_delta_throttle=10, max_delta_steering=10)
        state = PILOT.ControlState()

        command = PILOT.apply_safety_gate((15.0, 20.0), self.sample_latest(park=1), limits, state)

        self.assertEqual((command.throttle, command.steering), (0, 0))
        self.assertEqual(command.reason, "park_locked")

    def test_safety_gate_forces_zero_throttle_in_semi_auto(self):
        limits = PILOT.ControlLimits(max_throttle=20, max_steering=30, max_delta_throttle=10, max_delta_steering=10)
        state = PILOT.ControlState()

        command = PILOT.apply_safety_gate((15.0, 20.0), self.sample_latest(mode=1), limits, state)

        self.assertEqual(command.throttle, 0)
        self.assertGreater(command.steering, 0)
        self.assertEqual(command.reason, "semi_auto")

    def test_safety_gate_limits_live_output(self):
        limits = PILOT.ControlLimits(max_throttle=10, max_steering=20, max_delta_throttle=5, max_delta_steering=7)
        state = PILOT.ControlState(last_throttle=0, last_steering=0)

        command = PILOT.apply_safety_gate((80.0, -90.0), self.sample_latest(mode=2), limits, state)

        self.assertEqual((command.throttle, command.steering), (5, -7))

    def test_safety_gate_blocks_non_finite_prediction(self):
        limits = PILOT.ControlLimits(max_throttle=20, max_steering=30, max_delta_throttle=10, max_delta_steering=10)
        state = PILOT.ControlState()

        command = PILOT.apply_safety_gate((math.nan, 1.0), self.sample_latest(mode=2), limits, state)

        self.assertEqual((command.throttle, command.steering), (0, 0))
        self.assertEqual(command.reason, "non_finite")

    def test_format_serial_command_uses_sequence(self):
        self.assertEqual(PILOT.format_serial_command(1, -2, 7), "1:-2:7")

    def test_parse_ack_accepts_sequence_ack(self):
        self.assertTrue(PILOT.parse_ack("ACK:7", seq=7, ack_mode="seq"))
        self.assertFalse(PILOT.parse_ack("ACK:8", seq=7, ack_mode="seq"))
        self.assertFalse(PILOT.parse_ack("NACK:7", seq=7, ack_mode="seq"))

    def test_zero_output_forces_zero_command(self):
        command = PILOT.command_for_mode("zero-output", PILOT.ControlCommand(5, -4, "live"), seq=3)

        self.assertEqual(command, PILOT.ControlCommand(0, 0, "zero_output"))

    def test_live_requires_explicit_risk_acknowledgement(self):
        args = SimpleNamespace(mode="live", i_understand_risk=False)

        with self.assertRaises(ValueError):
            PILOT.validate_runtime_mode(args)

    def test_dry_run_does_not_open_serial(self):
        args = SimpleNamespace(mode="dry-run", serial_port=None)
        serial_factory = MagicMock()

        serial_obj = PILOT.open_serial_if_needed(args, serial_factory=serial_factory)

        self.assertIsNone(serial_obj)
        serial_factory.assert_not_called()


if __name__ == "__main__":
    unittest.main()
