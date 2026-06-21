"""测试 tools/transform_mus4_tub_to_donkey.py。"""

from __future__ import annotations

import importlib.util
import json
import os
import pathlib
import tempfile
import time
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE_PATH = PROJECT_ROOT / "tools" / "transform_mus4_tub_to_donkey.py"
SPEC = importlib.util.spec_from_file_location("transform_mus4_tub_to_donkey", MODULE_PATH)
TRANSFORM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TRANSFORM)


def make_raw_sample(seq: int, t_ms: int) -> dict:
    return {
        "seq": seq,
        "t": t_ms,
        "dt": 21,
        "thr": 42,
        "str": -85,
        "gz": 0.123,
        "gx": 0.011,
        "gy": -0.022,
        "ax": 0.5,
        "ay": -0.3,
        "az": 9.78,
        "mode": 0,
        "park": 0,
        "ch1": 1500,
        "ch2": 1500,
        "ch3": 1000,
        "ch4": 1000,
        "ch5": 2000,
        "ch6": 1500,
        "rct": 1500,
        "rcs": 1500,
        "pt": 0,
        "ps": 0,
        "gzf": 0.123,
        "dc": 0,
        "de": 1,
        "da": 0,
        "vol": 12.0,
    }


def make_raw_tub(sample_count: int, start_ms: int = 1000) -> dict:
    samples = [make_raw_sample(seq=i, t_ms=start_ms + i * 21) for i in range(sample_count)]
    return {
        "schema": "mus4.web_data_point.tub.v2",
        "source": "mus4-web-console",
        "started_ms": samples[0]["t"],
        "stopped_ms": samples[-1]["t"],
        "count": sample_count,
        "samples": samples,
    }


def write_reference_tub(ref_dir: pathlib.Path, image_count: int, per_catalog: int = 1000) -> None:
    """构造一个迷你的参考 tub，仅用于提供 cam/image_array 文件名。"""
    ref_dir.mkdir(parents=True, exist_ok=True)
    catalog_paths: list[str] = []
    for catalog_index, start in enumerate(range(0, image_count, per_catalog)):
        chunk_end = min(start + per_catalog, image_count)
        catalog_name = f"catalog_{catalog_index}.catalog"
        with (ref_dir / catalog_name).open("w", encoding="utf-8", newline="\n") as handle:
            for idx in range(start, chunk_end):
                record = {
                    "_index": idx,
                    "_session_id": "26-06-20_0",
                    "_timestamp_ms": 1700000000000 + idx,
                    "cam/image_array": f"{idx}_cam_image_array_.jpg",
                    "user/angle": 0.0,
                    "user/mode": "user",
                    "user/throttle": 0.0,
                }
                handle.write(json.dumps(record) + "\n")
        catalog_paths.append(catalog_name)
    (ref_dir / "manifest.json").write_text(
        "\n".join([
            json.dumps(["cam/image_array", "user/angle", "user/throttle", "user/mode"]),
            json.dumps(["image_array", "float", "float", "str"]),
            "{}",
            json.dumps({"created_at": 1700000000.0, "sessions": {}}),
            json.dumps({"paths": catalog_paths, "current_index": image_count, "max_len": per_catalog, "deleted_indexes": []}),
        ]) + "\n",
        encoding="utf-8",
    )


class TestTransform(unittest.TestCase):
    def _write_raw(self, tmpdir: pathlib.Path, sample_count: int, mtime_epoch: float) -> pathlib.Path:
        path = tmpdir / "raw.json"
        path.write_text(json.dumps(make_raw_tub(sample_count)), encoding="utf-8")
        os.utime(path, (mtime_epoch, mtime_epoch))
        return path

    def test_transform_truncate_drops_excess_raw_samples_when_ref_smaller(self):
        """truncate 策略下，超过参考数量的 raw sample 应被丢弃。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=10, mtime_epoch=1700000000.0)
            ref_dir = tmp / "ref"
            write_reference_tub(ref_dir, image_count=6)
            out_dir = tmp / "trans"

            stats = TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=ref_dir,
                cam_strategy="truncate",
                max_len=1000,
            )

            self.assertEqual(stats["raw_sample_count"], 10)
            self.assertEqual(stats["record_count"], 6)
            self.assertEqual(stats["dropped_count"], 4)

    def test_transform_cycle_keeps_all_raw_samples(self):
        """cycle 策略下，所有 raw sample 都保留，cam 名称循环复用。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=10, mtime_epoch=1700000000.0)
            ref_dir = tmp / "ref"
            write_reference_tub(ref_dir, image_count=3)
            out_dir = tmp / "trans"

            TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=ref_dir,
                cam_strategy="cycle",
                max_len=1000,
            )

            catalog = (out_dir / "catalog_0.catalog").read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(catalog), 10)
            names = [json.loads(line)["cam/image_array"] for line in catalog]
            self.assertEqual(names[0], "0_cam_image_array_.jpg")
            self.assertEqual(names[3], "0_cam_image_array_.jpg")  # 循环回到起点

    def test_transform_placeholder_does_not_require_ref(self):
        """placeholder 策略下，无需参考目录也能跑通。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=4, mtime_epoch=1700000000.0)
            out_dir = tmp / "trans"

            TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=None,
                cam_strategy="placeholder",
                max_len=1000,
            )

            catalog = (out_dir / "catalog_0.catalog").read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(catalog), 4)
            first = json.loads(catalog[0])
            self.assertEqual(first["cam/image_array"], "0_cam_image_array_.jpg")

    def test_record_field_mapping_matches_donkey_prompt(self):
        """转换出的 record 字段与 Donkey 提示词一致：thr/100→throttle，str/100→angle，imu 同名映射。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=1, mtime_epoch=1700000010.0)
            out_dir = tmp / "trans"

            TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=None,
                cam_strategy="placeholder",
                max_len=1000,
            )

            line = (out_dir / "catalog_0.catalog").read_text(encoding="utf-8").splitlines()[0]
            record = json.loads(line)
            # 字段集合
            for key in (
                "cam/image_array", "user/angle", "user/throttle", "user/mode",
                "imu/accel_x", "imu/accel_y", "imu/accel_z",
                "imu/gyro_x", "imu/gyro_y", "imu/gyro_z", "ts",
            ):
                self.assertIn(key, record)
            # 数值映射
            self.assertAlmostEqual(record["user/throttle"], 0.42, places=6)
            self.assertAlmostEqual(record["user/angle"], -0.85, places=6)
            self.assertEqual(record["user/mode"], "user")
            self.assertAlmostEqual(record["imu/accel_y"], -0.3, places=6)
            self.assertAlmostEqual(record["imu/accel_z"], 9.78, places=6)
            self.assertAlmostEqual(record["imu/gyro_z"], 0.123, places=6)

    def test_ts_is_derived_from_raw_mtime(self):
        """ts 应由 raw mtime 反推：单样本时 ts == mtime（因为 started==stopped）。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            mtime_epoch = 1700001234.567
            raw_path = self._write_raw(tmp, sample_count=1, mtime_epoch=mtime_epoch)
            out_dir = tmp / "trans"

            TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=None,
                cam_strategy="placeholder",
                max_len=1000,
            )

            line = (out_dir / "catalog_0.catalog").read_text(encoding="utf-8").splitlines()[0]
            record = json.loads(line)
            # 允许 fs mtime 精度差 1ms
            self.assertAlmostEqual(record["ts"], mtime_epoch, delta=0.01)

    def test_catalog_split_and_manifest_layout_match_donkey_reference(self):
        """输出目录应含 catalog_N.catalog + .catalog_manifest + manifest.json，且 max_len 切分正确。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=2500, mtime_epoch=1700000000.0)
            out_dir = tmp / "trans"

            stats = TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=None,
                cam_strategy="placeholder",
                max_len=1000,
            )

            self.assertEqual(stats["catalog_count"], 3)
            for index in range(3):
                self.assertTrue((out_dir / f"catalog_{index}.catalog").exists())
                self.assertTrue((out_dir / f"catalog_{index}.catalog_manifest").exists())
            top_manifest = (out_dir / "manifest.json").read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(top_manifest), 5)
            inputs = json.loads(top_manifest[0])
            self.assertIn("imu/gyro_z", inputs)
            self.assertIn("ts", inputs)
            paths_payload = json.loads(top_manifest[4])
            self.assertEqual(paths_payload["max_len"], 1000)
            self.assertEqual(paths_payload["current_index"], 2500)
            self.assertEqual(paths_payload["paths"], [
                "catalog_0.catalog", "catalog_1.catalog", "catalog_2.catalog",
            ])

    def test_catalog_manifest_line_lengths_exclude_newline(self):
        """`.catalog_manifest` 的 line_lengths 不含换行符（与 Donkey 参考一致）。"""
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            raw_path = self._write_raw(tmp, sample_count=3, mtime_epoch=1700000000.0)
            out_dir = tmp / "trans"

            TRANSFORM.transform(
                raw_path=raw_path,
                out_dir=out_dir,
                ref_dir=None,
                cam_strategy="placeholder",
                max_len=1000,
            )

            catalog = (out_dir / "catalog_0.catalog").read_text(encoding="utf-8").splitlines()
            manifest = json.loads((out_dir / "catalog_0.catalog_manifest").read_text(encoding="utf-8"))
            self.assertEqual(len(manifest["line_lengths"]), 3)
            for line, length in zip(catalog, manifest["line_lengths"]):
                self.assertEqual(length, len(line))


if __name__ == "__main__":
    unittest.main()
