#!/usr/bin/env python3
"""把 MUS4 Web Console 录制的 Tub v2 JSON 转成 DonkeyCar 原生 tub catalog 结构。

设计目标：
- 输入：单个 `mus4-tub*.json`（schema=`mus4.web_data_point.tub.v2`，含 IMU 五轴）。
- 参考：Donkey 原生 tub 目录（含 `catalog_*.catalog`、`*.catalog_manifest`、`manifest.json`）。
- 输出：与参考目录结构一致的 catalog 拆分（1000 条 / 文件 by 默认），可直接喂给 Donkey 训练管道。

主要映射：
- user/angle    = sample["str"] / 100.0   （MUS4 ±100 → Donkey ±1.0）
- user/throttle = sample["thr"] / 100.0
- user/mode     = "user"
- imu/accel_*   = sample["ax"/"ay"/"az"]   （m/s²，az 含重力 ~9.8，去重力交给训练侧）
- imu/gyro_*    = sample["gx"/"gy"/"gz"]   （rad/s）
- ts            = epoch 秒，由 raw 文件 mtime 反推：
                  epoch_start = mtime_epoch - (stopped_ms - started_ms) / 1000
                  ts_i        = epoch_start + (t_i - started_ms) / 1000

cam/image_array 处理（raw 没有相机）：按参考 catalog 的 `cam/image_array` 顺序背贴。
- truncate（默认）：超出参考数量的 raw 样本丢弃。
- cycle：参考耗尽后从头循环复用，保留所有 raw 样本。
- placeholder：纯字符串占位 `<index>_cam_image_array_.jpg`，不依赖参考目录。
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys
from typing import Iterable


DEFAULT_MAX_LEN_PER_CATALOG = 1000
TUB_SCHEMA_V2 = "mus4.web_data_point.tub.v2"


def load_raw_tub(path: pathlib.Path) -> dict:
    """读 MUS4 Tub v2 JSON。"""
    with path.open("r", encoding="utf-8") as handle:
        package = json.load(handle)
    schema = package.get("schema", "")
    if schema != TUB_SCHEMA_V2:
        # 不抛错，仅警告，方便用户拿历史 v1 数据先跑通脚本（v1 缺 IMU 五轴会被填 0）。
        print(
            f"[警告] schema={schema!r}，期望 {TUB_SCHEMA_V2!r}；缺失字段将填 0。",
            file=sys.stderr,
        )
    samples = package.get("samples") or []
    if not samples:
        raise ValueError(f"raw tub {path} 不含 samples")
    return package


def load_reference_image_names(ref_dir: pathlib.Path) -> list[str]:
    """按 _index 升序收集参考目录里的 cam/image_array 文件名。"""
    catalogs = sorted(ref_dir.glob("catalog_*.catalog"))
    if not catalogs:
        raise FileNotFoundError(f"参考目录 {ref_dir} 下没有 catalog_*.catalog")
    records: list[tuple[int, str]] = []
    for catalog in catalogs:
        with catalog.open("r", encoding="utf-8") as handle:
            for line in handle:
                line = line.strip()
                if not line:
                    continue
                record = json.loads(line)
                idx = record.get("_index")
                name = record.get("cam/image_array")
                if idx is None or not name:
                    continue
                records.append((idx, name))
    records.sort(key=lambda item: item[0])
    return [name for _, name in records]


def compute_epoch_start(raw_path: pathlib.Path, package: dict) -> float:
    """由 raw 文件 mtime 反推该批 sample 的 epoch 起点。

    约定：mtime ≈ stopped_ms 对应的 wall-clock。
    起点 epoch = mtime - (stopped_ms - started_ms) / 1000。
    """
    started_ms = float(package.get("started_ms") or 0)
    stopped_ms = float(package.get("stopped_ms") or 0)
    duration_s = max(0.0, (stopped_ms - started_ms) / 1000.0)
    mtime_epoch = raw_path.stat().st_mtime
    return mtime_epoch - duration_s


def map_image_array(
    sample_index: int,
    strategy: str,
    ref_image_names: list[str] | None,
) -> str:
    """按策略产出 cam/image_array 文件名。"""
    if strategy == "placeholder":
        return f"{sample_index}_cam_image_array_.jpg"
    if not ref_image_names:
        raise ValueError("策略 truncate/cycle 必须提供 --ref-dir")
    if strategy == "cycle":
        return ref_image_names[sample_index % len(ref_image_names)]
    if strategy == "truncate":
        if sample_index >= len(ref_image_names):
            return ""  # 调用方据此丢弃该样本
        return ref_image_names[sample_index]
    raise ValueError(f"未知 cam 策略：{strategy}")


def transform_sample(
    sample: dict,
    sample_index: int,
    epoch_start: float,
    started_ms: float,
    image_name: str,
    session_id: str,
) -> dict:
    """把 raw sample 转成 Donkey record。"""
    t_ms = float(sample.get("t") or 0.0)
    ts = epoch_start + (t_ms - started_ms) / 1000.0
    return {
        # 兼容 Donkey 原生顺序：先 _meta，再 cam，再 user/*，再 imu/*，最后 ts。
        "_index": sample_index,
        "_session_id": session_id,
        "_timestamp_ms": int(round(ts * 1000.0)),
        "cam/image_array": image_name,
        "user/angle": round(float(sample.get("str") or 0) / 100.0, 6),
        "user/throttle": round(float(sample.get("thr") or 0) / 100.0, 6),
        "user/mode": "user",
        "imu/accel_x": float(sample.get("ax") or 0.0),
        "imu/accel_y": float(sample.get("ay") or 0.0),
        "imu/accel_z": float(sample.get("az") or 0.0),
        "imu/gyro_x": float(sample.get("gx") or 0.0),
        "imu/gyro_y": float(sample.get("gy") or 0.0),
        "imu/gyro_z": float(sample.get("gz") or 0.0),
        "ts": round(ts, 3),
    }


def write_catalog_files(
    records: list[dict],
    out_dir: pathlib.Path,
    max_len: int,
    session_id: str,
    created_at: float,
) -> list[str]:
    """按 max_len 切分写入 catalog_*.catalog 与 .catalog_manifest，返回 catalog 路径列表。"""
    out_dir.mkdir(parents=True, exist_ok=True)
    catalog_paths: list[str] = []
    for catalog_index, start in enumerate(range(0, len(records), max_len)):
        chunk = records[start : start + max_len]
        catalog_name = f"catalog_{catalog_index}.catalog"
        catalog_path = out_dir / catalog_name
        line_lengths: list[int] = []
        with catalog_path.open("w", encoding="utf-8", newline="\n") as handle:
            for record in chunk:
                line = json.dumps(record, ensure_ascii=False)
                handle.write(line + "\n")
                # Donkey 的 line_lengths 不含换行符（与参考目录一致）。
                line_lengths.append(len(line))
        manifest_path = out_dir / f"{catalog_name}_manifest"
        manifest_payload = {
            "created_at": created_at,
            "line_lengths": line_lengths,
            "path": f"{catalog_name}_manifest",
            "start_index": start,
        }
        manifest_path.write_text(
            json.dumps(manifest_payload, ensure_ascii=False),
            encoding="utf-8",
        )
        catalog_paths.append(catalog_name)
    return catalog_paths


def write_top_manifest(
    out_dir: pathlib.Path,
    catalog_paths: list[str],
    record_count: int,
    max_len: int,
    session_id: str,
    created_at: float,
) -> None:
    """写 tub 顶层 manifest.json，结构与参考目录一致（4 行元 + 1 行 sessions + 1 行 paths）。"""
    inputs = ["cam/image_array", "user/angle", "user/throttle", "user/mode",
              "imu/accel_x", "imu/accel_y", "imu/accel_z",
              "imu/gyro_x", "imu/gyro_y", "imu/gyro_z", "ts"]
    types = ["image_array", "float", "float", "str",
             "float", "float", "float",
             "float", "float", "float",
             "float"]
    metadata: dict = {}
    sessions = {
        "all_full_ids": [session_id],
        "last_id": int(session_id.split("_")[-1]) if "_" in session_id else 0,
        "last_full_id": session_id,
    }
    paths_payload = {
        "paths": catalog_paths,
        "current_index": record_count,
        "max_len": max_len,
        "deleted_indexes": [],
    }
    lines = [
        json.dumps(inputs, ensure_ascii=False),
        json.dumps(types, ensure_ascii=False),
        json.dumps(metadata, ensure_ascii=False),
        json.dumps({"created_at": created_at, "sessions": sessions}, ensure_ascii=False),
        json.dumps(paths_payload, ensure_ascii=False),
    ]
    (out_dir / "manifest.json").write_text("\n".join(lines) + "\n", encoding="utf-8")


def build_session_id(epoch_start: float) -> str:
    """按 raw 录制起点 epoch 生成 session id，风格与参考 `26-06-20_0` 一致。"""
    import datetime as _dt

    dt = _dt.datetime.fromtimestamp(epoch_start)
    return dt.strftime("%y-%m-%d_0")


def transform(
    raw_path: pathlib.Path,
    out_dir: pathlib.Path,
    ref_dir: pathlib.Path | None,
    cam_strategy: str,
    max_len: int,
) -> dict:
    """主转换流程，返回统计信息。"""
    package = load_raw_tub(raw_path)
    samples = package["samples"]
    started_ms = float(package.get("started_ms") or 0)
    epoch_start = compute_epoch_start(raw_path, package)
    session_id = build_session_id(epoch_start)
    ref_image_names: list[str] | None = None
    if cam_strategy in ("truncate", "cycle"):
        if ref_dir is None:
            raise ValueError("策略 truncate/cycle 必须传 --ref-dir")
        ref_image_names = load_reference_image_names(ref_dir)

    records: list[dict] = []
    dropped = 0
    for idx, sample in enumerate(samples):
        image_name = map_image_array(idx, cam_strategy, ref_image_names)
        if cam_strategy == "truncate" and not image_name:
            dropped += 1
            continue
        records.append(
            transform_sample(
                sample,
                sample_index=len(records),
                epoch_start=epoch_start,
                started_ms=started_ms,
                image_name=image_name,
                session_id=session_id,
            )
        )

    created_at = epoch_start  # 与 Donkey 风格保持一致：tub 创建时间 ≈ 录制起点
    catalog_paths = write_catalog_files(
        records, out_dir, max_len, session_id, created_at
    )
    write_top_manifest(
        out_dir, catalog_paths, len(records), max_len, session_id, created_at,
    )
    return {
        "raw_sample_count": len(samples),
        "record_count": len(records),
        "dropped_count": dropped,
        "catalog_count": len(catalog_paths),
        "session_id": session_id,
        "epoch_start": epoch_start,
        "ref_image_count": len(ref_image_names) if ref_image_names else 0,
    }


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="把 MUS4 Tub v2 JSON 转成 DonkeyCar tub catalog 目录。",
    )
    parser.add_argument("raw", help="输入的 MUS4 Tub JSON（schema v2）")
    parser.add_argument(
        "--out-dir",
        required=True,
        help="输出 tub 目录（catalog_*.catalog + manifest.json）",
    )
    parser.add_argument(
        "--ref-dir",
        default=None,
        help="DonkeyCar 参考 tub 目录（提供 cam/image_array 文件名）",
    )
    parser.add_argument(
        "--cam-strategy",
        default="truncate",
        choices=("truncate", "cycle", "placeholder"),
        help="cam/image_array 来源策略：truncate=只用参考能覆盖的样本；"
        "cycle=循环复用参考名；placeholder=纯字符串占位。",
    )
    parser.add_argument(
        "--max-len",
        type=int,
        default=DEFAULT_MAX_LEN_PER_CATALOG,
        help=f"每个 catalog 文件最多保留多少条 record（默认 {DEFAULT_MAX_LEN_PER_CATALOG}）",
    )
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    raw_path = pathlib.Path(args.raw)
    out_dir = pathlib.Path(args.out_dir)
    ref_dir = pathlib.Path(args.ref_dir) if args.ref_dir else None
    if not raw_path.is_file():
        print(f"[错误] 输入文件不存在：{raw_path}", file=sys.stderr)
        return 2

    stats = transform(
        raw_path=raw_path,
        out_dir=out_dir,
        ref_dir=ref_dir,
        cam_strategy=args.cam_strategy,
        max_len=args.max_len,
    )
    print(
        "✅ 转换完成：\n"
        f"  原始样本数  : {stats['raw_sample_count']}\n"
        f"  输出 record : {stats['record_count']}（丢弃 {stats['dropped_count']}）\n"
        f"  catalog 数  : {stats['catalog_count']}\n"
        f"  session_id  : {stats['session_id']}\n"
        f"  epoch 起点  : {stats['epoch_start']:.3f}（由 raw mtime 反推）\n"
        f"  参考图片数  : {stats['ref_image_count']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
