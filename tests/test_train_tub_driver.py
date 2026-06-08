import importlib.util
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "tools" / "train_tub_driver.py"
SPEC = importlib.util.spec_from_file_location("train_tub_driver", MODULE_PATH)
TRAIN_TUB_DRIVER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TRAIN_TUB_DRIVER)


def make_sample(seq, t, throttle=5, steering=-3):
    return {
        "seq": seq,
        "t": t,
        "dt": 22,
        "thr": throttle,
        "str": steering,
        "gz": 0.1 * seq,
        "mode": 0,
        "park": 0,
        "ch1": 1500 + seq,
        "ch2": 1500 - seq,
        "ch3": 1000,
        "ch4": 1000,
        "ch5": 2000,
        "ch6": 1372,
        "rct": 1500 - seq,
        "rcs": 1500 + seq,
        "pt": 0,
        "ps": 0,
        "gzf": 0.1 * seq,
        "dc": 1.5,
        "de": 1,
        "da": 0,
        "vol": 11.1,
    }


def make_tub(samples):
    return {
        "schema": "mus4.web_data_point.tub.v1",
        "source": "mus4-web-console",
        "started_ms": samples[0]["t"] if samples else 0,
        "stopped_ms": samples[-1]["t"] if samples else 0,
        "count": len(samples),
        "samples": samples,
    }


class TestTrainTubDriver(unittest.TestCase):
    def write_tub(self, directory, name="sample.json", samples=None):
        samples = samples or [make_sample(1, 1000), make_sample(2, 1022), make_sample(3, 1044)]
        path = pathlib.Path(directory) / name
        path.write_text(json.dumps(make_tub(samples)), encoding="utf-8")
        return path

    def test_loads_tub_json_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = self.write_tub(tmpdir)

            package = TRAIN_TUB_DRIVER.load_tub_json(path)

            self.assertEqual(package["count"], 3)
            self.assertEqual(len(package["samples"]), 3)

    def test_validate_tub_package_reports_count_mismatch(self):
        package = make_tub([make_sample(1, 1000), make_sample(2, 1022)])
        package["count"] = 99

        report = TRAIN_TUB_DRIVER.validate_tub_package(package, source_path="bad.json")

        self.assertTrue(report["ok"])
        self.assertTrue(any("count" in warning for warning in report["warnings"]))

    def test_default_feature_columns_exclude_leakage_fields(self):
        samples = TRAIN_TUB_DRIVER.normalize_samples(make_tub([make_sample(1, 1000), make_sample(2, 1022)]))

        feature_columns = TRAIN_TUB_DRIVER.select_feature_columns(
            samples,
            label_columns=["thr", "str"],
            exclude_columns=TRAIN_TUB_DRIVER.DEFAULT_EXCLUDE_COLUMNS,
            add_exclude_columns=[],
        )

        for column in ["ch1", "ch2", "rct", "rcs", "thr", "str", "seq", "t"]:
            self.assertNotIn(column, feature_columns)
        self.assertIn("gz", feature_columns)
        self.assertIn("vol", feature_columns)

    def test_build_window_dataset_aligns_labels(self):
        samples = [make_sample(i, 1000 + i * 22, throttle=i, steering=-i) for i in range(1, 6)]
        normalized = TRAIN_TUB_DRIVER.normalize_samples(make_tub(samples))
        feature_columns = ["gz", "vol"]

        windows, labels, meta = TRAIN_TUB_DRIVER.build_window_dataset(
            [normalized],
            feature_columns=feature_columns,
            label_columns=["thr", "str"],
            window_size=3,
            stride=1,
            target_offset=0,
        )

        self.assertEqual(len(windows), 3)
        self.assertEqual(len(windows[0]), 3)
        self.assertEqual(len(windows[0][0]), len(feature_columns))
        self.assertEqual(labels[0], [3.0, -3.0])
        self.assertEqual(meta[0]["source_sample_index"], 2)

    def test_build_window_dataset_does_not_cross_package_boundaries(self):
        first = TRAIN_TUB_DRIVER.normalize_samples(make_tub([make_sample(1, 1000), make_sample(2, 1022), make_sample(3, 1044)]))
        second = TRAIN_TUB_DRIVER.normalize_samples(make_tub([make_sample(10, 2000), make_sample(11, 2022), make_sample(12, 2044)]))

        windows, labels, meta = TRAIN_TUB_DRIVER.build_window_dataset(
            [first, second],
            feature_columns=["gz", "vol"],
            label_columns=["thr", "str"],
            window_size=3,
            stride=1,
            target_offset=0,
        )

        self.assertEqual(len(windows), 2)
        self.assertEqual([item["source_index"] for item in meta], [0, 1])

    def test_build_quality_report_contains_training_summary(self):
        samples = TRAIN_TUB_DRIVER.normalize_samples(make_tub([make_sample(i, 1000 + i * 22) for i in range(1, 6)]))
        windows, labels, meta = TRAIN_TUB_DRIVER.build_window_dataset(
            [samples],
            feature_columns=["gz", "vol"],
            label_columns=["thr", "str"],
            window_size=3,
            stride=1,
            target_offset=0,
        )

        report = TRAIN_TUB_DRIVER.build_quality_report(
            packages=[make_tub(samples)],
            validation_reports=[TRAIN_TUB_DRIVER.validate_tub_package(make_tub(samples))],
            samples_by_package=[samples],
            feature_columns=["gz", "vol"],
            label_columns=["thr", "str"],
            excluded_columns=["ch1", "ch2", "rct", "rcs", "thr", "str"],
            window_count=len(windows),
        )

        self.assertEqual(report["sample_count"], 5)
        self.assertEqual(report["window_count"], 3)
        self.assertEqual(report["feature_columns"], ["gz", "vol"])
        self.assertIn("estimated_hz", report)
        self.assertIn("thr", report["label_stats"])

    def test_cli_dry_run_does_not_require_torch(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = self.write_tub(tmpdir, samples=[make_sample(i, 1000 + i * 22) for i in range(1, 8)])

            result = subprocess.run(
                [sys.executable, str(MODULE_PATH), str(path), "--dry-run", "--window-size", "3"],
                text=True,
                capture_output=True,
                cwd=PROJECT_ROOT,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("样本数", result.stdout)
            self.assertIn("窗口数", result.stdout)
            self.assertNotIn("No module named 'torch'", result.stderr)


if __name__ == "__main__":
    unittest.main()
