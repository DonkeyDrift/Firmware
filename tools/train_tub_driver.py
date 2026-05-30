#!/usr/bin/env python3
"""MUS4 tub JSON 行为克隆 baseline 训练工具。"""

from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
import sys
from pathlib import Path
from typing import Iterable, Sequence


SCHEMA = "mus4.web_data_point.tub.v1"
REPORT_SCHEMA = "mus4.tub_driver_training_report.v1"
STANDARDIZATION_SCHEMA = "mus4.tub_driver_standardization.v1"
DEFAULT_LABEL_COLUMNS = ("thr", "str")
DEFAULT_EXCLUDE_COLUMNS = ("ch1", "ch2", "rct", "rcs", "thr", "str")
DEFAULT_META_COLUMNS = ("seq", "t")
PREFERRED_FEATURE_ORDER = (
    "dt",
    "gz",
    "gzf",
    "mode",
    "park",
    "ch3",
    "ch4",
    "ch5",
    "ch6",
    "pt",
    "ps",
    "dc",
    "de",
    "da",
    "vol",
)


def parse_csv_columns(value: str | None) -> list[str]:
    if not value:
        return []
    return [item.strip() for item in value.split(",") if item.strip()]


def load_tub_json(path: str | Path) -> dict:
    tub_path = Path(path)
    try:
        with tub_path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except json.JSONDecodeError as exc:
        raise ValueError(f"无法解析 tub JSON：{tub_path}：{exc}") from exc


def load_tub_jsons(paths: Sequence[str | Path]) -> list[dict]:
    return [load_tub_json(path) for path in paths]


def _is_number(value) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(float(value))


def _to_number(value, default: float = 0.0) -> float:
    if value is None:
        return default
    if isinstance(value, bool):
        return float(int(value))
    try:
        number = float(value)
    except (TypeError, ValueError):
        return default
    if not math.isfinite(number):
        return default
    return number


def validate_tub_package(package: dict, source_path: str = "") -> dict:
    warnings: list[str] = []
    errors: list[str] = []
    if not isinstance(package, dict):
        return {"ok": False, "source_path": source_path, "warnings": warnings, "errors": ["tub 顶层不是对象"]}

    if package.get("schema") != SCHEMA:
        warnings.append(f"schema 不匹配：{package.get('schema')!r}")
    samples = package.get("samples")
    if not isinstance(samples, list):
        errors.append("samples 不是数组")
        samples = []
    if package.get("count") != len(samples):
        warnings.append(f"count 与 samples 长度不一致：count={package.get('count')} len={len(samples)}")

    required = ("t", "thr", "str")
    missing_required = {key: 0 for key in required}
    for sample in samples:
        if not isinstance(sample, dict):
            errors.append("samples 中存在非对象样本")
            continue
        for key in required:
            if key not in sample:
                missing_required[key] += 1
    for key, count in missing_required.items():
        if count:
            warnings.append(f"关键字段 {key} 缺失 {count} 次")

    times = [_to_number(sample.get("t")) for sample in samples if isinstance(sample, dict) and "t" in sample]
    if len(times) >= 2 and any(next_time < current for current, next_time in zip(times, times[1:])):
        warnings.append("时间戳 t 非单调递增")

    return {
        "ok": not errors,
        "source_path": source_path,
        "sample_count": len(samples),
        "warnings": warnings,
        "errors": errors,
    }


def normalize_samples(package: dict) -> list[dict]:
    normalized: list[dict] = []
    for sample in package.get("samples", []):
        if not isinstance(sample, dict):
            continue
        row = {}
        for key, value in sample.items():
            if isinstance(value, bool):
                row[key] = int(value)
            elif isinstance(value, (int, float)):
                row[key] = value
            else:
                try:
                    number = float(value)
                except (TypeError, ValueError):
                    row[key] = value
                else:
                    row[key] = number
        normalized.append(row)
    return normalized


def infer_numeric_columns(samples: Sequence[dict]) -> list[str]:
    columns: set[str] = set()
    non_numeric: set[str] = set()
    for sample in samples:
        for key, value in sample.items():
            if _is_number(value):
                columns.add(key)
            else:
                non_numeric.add(key)
    return sorted(columns - non_numeric)


def select_feature_columns(
    samples: Sequence[dict],
    label_columns: Sequence[str],
    exclude_columns: Sequence[str],
    add_exclude_columns: Sequence[str] | None = None,
) -> list[str]:
    numeric = set(infer_numeric_columns(samples))
    excluded = set(label_columns) | set(exclude_columns) | set(add_exclude_columns or []) | set(DEFAULT_META_COLUMNS)
    available = numeric - excluded
    ordered = [column for column in PREFERRED_FEATURE_ORDER if column in available]
    ordered.extend(sorted(available - set(ordered)))
    return ordered


def build_window_dataset(
    samples_by_package: Sequence[Sequence[dict]],
    feature_columns: Sequence[str],
    label_columns: Sequence[str],
    window_size: int = 16,
    stride: int = 1,
    target_offset: int = 0,
) -> tuple[list[list[list[float]]], list[list[float]], list[dict]]:
    if window_size <= 0:
        raise ValueError("window_size 必须大于 0")
    if stride <= 0:
        raise ValueError("stride 必须大于 0")
    windows: list[list[list[float]]] = []
    labels: list[list[float]] = []
    meta: list[dict] = []
    for source_index, samples in enumerate(samples_by_package):
        last_target = len(samples) - target_offset
        for end in range(window_size - 1, last_target, stride):
            target_index = end + target_offset
            window = []
            for row in samples[end - window_size + 1 : end + 1]:
                window.append([_to_number(row.get(column)) for column in feature_columns])
            label = [_to_number(samples[target_index].get(column)) for column in label_columns]
            windows.append(window)
            labels.append(label)
            meta.append(
                {
                    "source_index": source_index,
                    "source_sample_index": target_index,
                    "t": samples[target_index].get("t", 0),
                }
            )
    return windows, labels, meta


def compute_standardization(windows: Sequence[Sequence[Sequence[float]]]) -> dict:
    if not windows:
        return {"mean": [], "std": []}
    feature_dim = len(windows[0][0]) if windows[0] else 0
    mean: list[float] = []
    std: list[float] = []
    for feature_index in range(feature_dim):
        values = [step[feature_index] for window in windows for step in window]
        feature_mean = statistics.fmean(values) if values else 0.0
        feature_std = statistics.pstdev(values) if len(values) > 1 else 1.0
        mean.append(feature_mean)
        std.append(feature_std if feature_std > 1e-6 else 1.0)
    return {"mean": mean, "std": std}


def apply_standardization(windows: Sequence[Sequence[Sequence[float]]], stats: dict) -> list[list[list[float]]]:
    mean = stats.get("mean", [])
    std = stats.get("std", [])
    standardized: list[list[list[float]]] = []
    for window in windows:
        standardized.append([[((value - mean[i]) / std[i]) for i, value in enumerate(step)] for step in window])
    return standardized


def _series_stats(values: Sequence[float]) -> dict:
    if not values:
        return {"min": None, "max": None, "mean": None, "std": None}
    return {
        "min": min(values),
        "max": max(values),
        "mean": statistics.fmean(values),
        "std": statistics.pstdev(values) if len(values) > 1 else 0.0,
    }


def build_quality_report(
    packages: Sequence[dict],
    validation_reports: Sequence[dict],
    samples_by_package: Sequence[Sequence[dict]],
    feature_columns: Sequence[str],
    label_columns: Sequence[str],
    excluded_columns: Sequence[str],
    window_count: int,
) -> dict:
    all_samples = [sample for samples in samples_by_package for sample in samples]
    times = [_to_number(sample.get("t")) for sample in all_samples if "t" in sample]
    gaps = [b - a for a, b in zip(times, times[1:]) if b >= a]
    mean_gap = statistics.fmean(gaps) if gaps else 0.0
    estimated_hz = 1000.0 / mean_gap if mean_gap > 0 else None
    label_stats = {
        column: _series_stats([_to_number(sample.get(column)) for sample in all_samples if column in sample])
        for column in label_columns
    }
    warnings = [warning for report in validation_reports for warning in report.get("warnings", [])]
    if len(all_samples) < 3000:
        warnings.append("样本数量偏少，仅适合验证训练管线和 baseline")
    if any(column in {"pt", "ps"} for column in feature_columns):
        warnings.append("pt/ps 可能在自动/半自动场景造成泄漏，可用 --add-exclude-columns pt,ps 排除")
    return {
        "schema": REPORT_SCHEMA,
        "package_count": len(packages),
        "sample_count": len(all_samples),
        "window_count": window_count,
        "estimated_hz": estimated_hz,
        "dt_ms": _series_stats(gaps),
        "feature_columns": list(feature_columns),
        "label_columns": list(label_columns),
        "excluded_columns": list(excluded_columns),
        "label_stats": label_stats,
        "mode_distribution": _distribution(all_samples, "mode"),
        "park_distribution": _distribution(all_samples, "park"),
        "validation": list(validation_reports),
        "warnings": warnings,
    }


def _distribution(samples: Sequence[dict], column: str) -> dict:
    counts: dict[str, int] = {}
    for sample in samples:
        if column in sample:
            key = str(sample[column])
            counts[key] = counts.get(key, 0) + 1
    return counts


def prepare_dataset(args) -> dict:
    input_paths = [Path(path) for path in args.inputs]
    packages = load_tub_jsons(input_paths)
    validation_reports = [validate_tub_package(package, str(path)) for package, path in zip(packages, input_paths)]
    bad = [report for report in validation_reports if not report["ok"]]
    if bad:
        raise ValueError(f"tub 结构错误：{bad[0]['errors']}")
    samples_by_package = [normalize_samples(package) for package in packages]
    if args.max_samples:
        samples_by_package = [samples[: args.max_samples] for samples in samples_by_package]
    all_samples = [sample for samples in samples_by_package for sample in samples]
    label_columns = parse_csv_columns(args.label_columns) or list(DEFAULT_LABEL_COLUMNS)
    exclude_columns = parse_csv_columns(args.exclude_columns) if args.exclude_columns else list(DEFAULT_EXCLUDE_COLUMNS)
    add_exclude_columns = parse_csv_columns(args.add_exclude_columns)
    feature_columns = select_feature_columns(all_samples, label_columns, exclude_columns, add_exclude_columns)
    windows, labels, meta = build_window_dataset(
        samples_by_package,
        feature_columns,
        label_columns,
        window_size=args.window_size,
        stride=args.stride,
        target_offset=args.target_offset,
    )
    excluded_columns = list(dict.fromkeys(exclude_columns + add_exclude_columns + list(DEFAULT_META_COLUMNS)))
    report = build_quality_report(
        packages,
        validation_reports,
        samples_by_package,
        feature_columns,
        label_columns,
        excluded_columns,
        len(windows),
    )
    return {
        "input_paths": input_paths,
        "packages": packages,
        "samples_by_package": samples_by_package,
        "feature_columns": feature_columns,
        "label_columns": label_columns,
        "excluded_columns": excluded_columns,
        "windows": windows,
        "labels": labels,
        "meta": meta,
        "report": report,
    }


def print_summary(dataset: dict, args) -> None:
    report = dataset["report"]
    print("MUS4 tub JSON 训练数据检查")
    print(f"输入文件数: {len(dataset['input_paths'])}")
    print(f"样本数: {report['sample_count']}")
    hz = report.get("estimated_hz")
    print(f"估算采样率: {hz:.2f} Hz" if hz else "估算采样率: unknown")
    print(f"窗口大小: {args.window_size}")
    print(f"窗口数: {report['window_count']}")
    print("标签列: " + ",".join(dataset["label_columns"]))
    print("特征列: " + ",".join(dataset["feature_columns"]))
    print("已排除字段: " + ",".join(dataset["excluded_columns"]))
    for warning in report.get("warnings", []):
        print(f"警告: {warning}")


def ensure_out_dir(path: str | Path | None, overwrite: bool) -> Path:
    out_dir = Path(path or "runs/tub_driver")
    if out_dir.exists() and any(out_dir.iterdir()) and not overwrite:
        raise FileExistsError(f"输出目录非空，请使用 --overwrite：{out_dir}")
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir


def write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def write_report_only(dataset: dict, out_dir: Path) -> None:
    write_json(out_dir / "report.json", dataset["report"])


def train_gru_baseline(dataset: dict, args, out_dir: Path) -> dict:
    try:
        import numpy as np
        import torch
        from torch import nn
        from torch.utils.data import DataLoader, TensorDataset
    except ModuleNotFoundError as exc:
        raise RuntimeError("需要安装训练依赖：pip install torch numpy matplotlib。dry-run 和 report-only 不需要 torch。") from exc

    random.seed(args.seed)
    torch.manual_seed(args.seed)
    windows = dataset["windows"]
    labels = dataset["labels"]
    if len(windows) < 2:
        raise ValueError("窗口数量不足，无法训练")
    stats = compute_standardization(windows)
    x = np.array(apply_standardization(windows, stats), dtype=np.float32)
    y = np.array(labels, dtype=np.float32) / 100.0
    split = max(1, int(len(x) * (1.0 - args.val_ratio)))
    if split >= len(x):
        split = len(x) - 1
    train_x, val_x = x[:split], x[split:]
    train_y, val_y = y[:split], y[split:]

    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available() else ("cpu" if args.device == "auto" else args.device))

    class DriverGRU(nn.Module):
        def __init__(self, input_dim: int):
            super().__init__()
            dropout = args.dropout if args.num_layers > 1 else 0.0
            self.gru = nn.GRU(input_dim, args.hidden_size, args.num_layers, batch_first=True, dropout=dropout)
            self.head = nn.Sequential(nn.Linear(args.hidden_size, args.hidden_size), nn.ReLU(), nn.Linear(args.hidden_size, 2), nn.Tanh())

        def forward(self, xb):
            out, _ = self.gru(xb)
            return self.head(out[:, -1, :])

    model = DriverGRU(len(dataset["feature_columns"])).to(device)
    train_loader = DataLoader(TensorDataset(torch.tensor(train_x), torch.tensor(train_y)), batch_size=args.batch_size, shuffle=False)
    val_tensor_x = torch.tensor(val_x).to(device)
    val_tensor_y = torch.tensor(val_y).to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.learning_rate)
    loss_fn = nn.MSELoss()
    history: list[dict] = []
    for epoch in range(1, args.epochs + 1):
        model.train()
        train_loss = 0.0
        seen = 0
        for xb, yb in train_loader:
            xb = xb.to(device)
            yb = yb.to(device)
            pred = model(xb)
            loss = loss_fn(pred, yb)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            train_loss += loss.item() * len(xb)
            seen += len(xb)
        model.eval()
        with torch.no_grad():
            val_pred = model(val_tensor_x)
            val_loss = loss_fn(val_pred, val_tensor_y).item()
        row = {"epoch": epoch, "train_loss": train_loss / max(1, seen), "val_loss": val_loss}
        history.append(row)
        print(f"epoch={epoch:03d} train_loss={row['train_loss']:.6f} val_loss={val_loss:.6f}")

    torch.save(
        {
            "model_state_dict": model.state_dict(),
            "config": {
                "input_size": len(dataset["feature_columns"]),
                "hidden_size": args.hidden_size,
                "num_layers": args.num_layers,
                "feature_columns": dataset["feature_columns"],
                "label_columns": dataset["label_columns"],
            },
        },
        out_dir / "model.pt",
    )
    standardization = {
        "schema": STANDARDIZATION_SCHEMA,
        "feature_columns": dataset["feature_columns"],
        "label_columns": dataset["label_columns"],
        "mean": stats["mean"],
        "std": stats["std"],
        "window_size": args.window_size,
        "stride": args.stride,
        "target_offset": args.target_offset,
        "exclude_columns": dataset["excluded_columns"],
    }
    write_json(out_dir / "standardization.json", standardization)
    with torch.no_grad():
        preds = model(val_tensor_x).cpu().numpy() * 100.0
    true = val_y * 100.0
    write_predictions(out_dir / "predictions.csv", dataset, split, true, preds)
    plot_warnings = []
    if not args.no_plots:
        plot_warnings = write_plots(out_dir, history, true, preds)
    train_summary = {"history": history, "final_val_loss": history[-1]["val_loss"], "plot_warnings": plot_warnings}
    report = dict(dataset["report"])
    report["training"] = train_summary
    write_json(out_dir / "report.json", report)
    return train_summary


def write_predictions(path: Path, dataset: dict, split: int, true, preds) -> None:
    meta = dataset["meta"][split:]
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "source_index", "source_sample_index", "t", "true_thr", "pred_thr", "true_str", "pred_str", "split"])
        for i, (m, y, p) in enumerate(zip(meta, true, preds)):
            writer.writerow([i, m["source_index"], m["source_sample_index"], m["t"], y[0], p[0], y[1], p[1], "val"])


def write_plots(out_dir: Path, history: Sequence[dict], true, preds) -> list[str]:
    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError:
        return ["未安装 matplotlib，已跳过 PNG 曲线输出"]
    epochs = [row["epoch"] for row in history]
    plt.figure(figsize=(8, 4))
    plt.plot(epochs, [row["train_loss"] for row in history], label="train")
    plt.plot(epochs, [row["val_loss"] for row in history], label="val")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "loss_curve.png")
    plt.close()
    plt.figure(figsize=(10, 6))
    plt.subplot(2, 1, 1)
    plt.plot(true[:, 0], label="true_thr")
    plt.plot(preds[:, 0], label="pred_thr")
    plt.legend()
    plt.subplot(2, 1, 2)
    plt.plot(true[:, 1], label="true_str")
    plt.plot(preds[:, 1], label="pred_str")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "prediction_curve.png")
    plt.close()
    return []


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="训练 MUS4 tub JSON GRU baseline")
    parser.add_argument("inputs", nargs="+", help="一个或多个 tub JSON 文件")
    parser.add_argument("--out-dir")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--report-only", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--window-size", type=int, default=16)
    parser.add_argument("--stride", type=int, default=1)
    parser.add_argument("--target-offset", type=int, default=0)
    parser.add_argument("--label-columns", default=",".join(DEFAULT_LABEL_COLUMNS))
    parser.add_argument("--exclude-columns", default=None)
    parser.add_argument("--add-exclude-columns", default="")
    parser.add_argument("--max-samples", type=int, default=None)
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--hidden-size", type=int, default=64)
    parser.add_argument("--num-layers", type=int, default=1)
    parser.add_argument("--dropout", type=float, default=0.0)
    parser.add_argument("--learning-rate", type=float, default=0.001)
    parser.add_argument("--val-ratio", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--device", default="auto")
    parser.add_argument("--no-plots", action="store_true")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    try:
        dataset = prepare_dataset(args)
        print_summary(dataset, args)
        if args.dry_run:
            return 0
        out_dir = ensure_out_dir(args.out_dir, args.overwrite)
        if args.report_only:
            write_report_only(dataset, out_dir)
            print(f"报告已写入: {out_dir / 'report.json'}")
            return 0
        train_gru_baseline(dataset, args, out_dir)
        print(f"训练产物已写入: {out_dir}")
        return 0
    except Exception as exc:  # noqa: BLE001 - CLI 需要输出用户可读错误
        print(f"错误: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
