#!/usr/bin/env python3
"""MUS4 模型 Pilot 推理控制器。"""

from __future__ import annotations

import argparse
import json
import math
import sys
import time
import urllib.request
from collections import deque
from pathlib import Path
from typing import Callable, NamedTuple, Sequence


LEAKAGE_COLUMNS = {"ch1", "ch2", "rct", "rcs", "thr", "str"}
DEFAULT_ESP32_URL = "http://192.168.3.39"


class ControlLimits(NamedTuple):
    max_throttle: int = 10
    max_steering: int = 30
    max_delta_throttle: int = 5
    max_delta_steering: int = 10


class ControlState(NamedTuple):
    last_throttle: int = 0
    last_steering: int = 0


class ControlCommand(NamedTuple):
    throttle: int
    steering: int
    reason: str


class DriverGRUConfig(NamedTuple):
    input_size: int
    hidden_size: int
    num_layers: int
    label_columns: list[str]
    feature_columns: list[str]


def load_standardization(model_dir: str | Path) -> dict:
    path = Path(model_dir) / "standardization.json"
    with path.open("r", encoding="utf-8") as handle:
        stats = json.load(handle)
    validate_standardization(stats)
    return stats


def validate_standardization(stats: dict) -> None:
    feature_columns = list(stats.get("feature_columns", []))
    if not feature_columns:
        raise ValueError("standardization.json 缺少 feature_columns")
    leaked = LEAKAGE_COLUMNS.intersection(feature_columns)
    if leaked:
        raise ValueError(f"模型输入包含泄漏字段：{sorted(leaked)}")
    mean = stats.get("mean", [])
    std = stats.get("std", [])
    if len(mean) != len(feature_columns) or len(std) != len(feature_columns):
        raise ValueError("standardization.json 的 mean/std 维度与 feature_columns 不一致")
    if int(stats.get("window_size", 0)) <= 0:
        raise ValueError("standardization.json 缺少有效 window_size")


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


def build_feature_vector(latest: dict, feature_columns: Sequence[str]) -> list[float]:
    leaked = LEAKAGE_COLUMNS.intersection(feature_columns)
    if leaked:
        raise ValueError(f"模型输入包含泄漏字段：{sorted(leaked)}")
    missing = [column for column in feature_columns if column not in latest]
    if missing:
        raise ValueError(f"latest 缺少特征字段：{missing}")
    return [_to_number(latest[column]) for column in feature_columns]


def standardize_window(window: Sequence[Sequence[float]], stats: dict) -> list[list[float]]:
    validate_standardization(stats)
    mean = stats["mean"]
    std = stats["std"]
    if any(len(step) != len(mean) for step in window):
        raise ValueError("窗口特征维度与标准化参数不一致")
    return [[(value - mean[index]) / (std[index] or 1.0) for index, value in enumerate(step)] for step in window]


def build_feature_window(latest: dict, stats: dict, history: deque[list[float]]) -> list[list[float]] | None:
    vector = build_feature_vector(latest, stats["feature_columns"])
    history.append(vector)
    window_size = int(stats["window_size"])
    while len(history) > window_size:
        history.popleft()
    if len(history) < window_size:
        return None
    return standardize_window(list(history), stats)


def fetch_latest_data(esp32_url: str, timeout: float = 1.0) -> dict:
    url = esp32_url.rstrip("/") + "/api/data"
    with urllib.request.urlopen(url, timeout=timeout) as response:
        payload = json.loads(response.read().decode("utf-8"))
    latest = payload.get("latest")
    if not isinstance(latest, dict):
        raise ValueError("/api/data 未返回 latest 对象")
    return latest


def _clip_int(value: float, minimum: int, maximum: int) -> int:
    return max(minimum, min(maximum, int(round(value))))


def _limit_delta(value: int, previous: int, maximum_delta: int) -> int:
    if maximum_delta <= 0:
        return value
    return max(previous - maximum_delta, min(previous + maximum_delta, value))


def apply_safety_gate(prediction: Sequence[float], latest: dict, limits: ControlLimits, state: ControlState) -> ControlCommand:
    if len(prediction) < 2 or not all(math.isfinite(float(value)) for value in prediction[:2]):
        return ControlCommand(0, 0, "non_finite")
    if int(_to_number(latest.get("park"))) == 1:
        return ControlCommand(0, 0, "park_locked")
    mode = int(_to_number(latest.get("mode")))
    if mode == 0:
        return ControlCommand(0, 0, "manual_mode")

    throttle = _clip_int(float(prediction[0]), -limits.max_throttle, limits.max_throttle)
    steering = _clip_int(float(prediction[1]), -limits.max_steering, limits.max_steering)
    if mode == 1:
        throttle = 0
        reason = "semi_auto"
    elif mode == 2:
        reason = "full_auto"
    else:
        return ControlCommand(0, 0, "unknown_mode")

    throttle = _limit_delta(throttle, state.last_throttle, limits.max_delta_throttle)
    steering = _limit_delta(steering, state.last_steering, limits.max_delta_steering)
    return ControlCommand(throttle, steering, reason)


def format_serial_command(throttle: int, steering: int, seq: int) -> str:
    return f"{throttle}:{steering}:{seq}"


def parse_ack(line: str, seq: int, ack_mode: str = "seq") -> bool:
    stripped = line.strip().upper()
    if stripped.startswith("NACK"):
        return False
    if ack_mode == "any":
        return stripped == "ACK" or stripped.startswith("ACK:")
    return stripped == f"ACK:{seq}"


def command_for_mode(mode: str, command: ControlCommand, seq: int) -> ControlCommand:
    if mode == "zero-output":
        return ControlCommand(0, 0, "zero_output")
    return command


def validate_runtime_mode(args) -> None:
    if args.mode == "live" and not getattr(args, "i_understand_risk", False):
        raise ValueError("live 模式必须显式传入 --i-understand-risk")


def open_serial_if_needed(args, serial_factory: Callable | None = None):
    if args.mode == "dry-run":
        return None
    if not args.serial_port:
        raise ValueError("zero-output/live 模式必须指定 --serial-port")
    if serial_factory is None:
        try:
            import serial
        except ModuleNotFoundError as exc:
            raise RuntimeError("需要安装 pyserial：pip install pyserial") from exc
        serial_factory = serial.Serial
    return serial_factory(args.serial_port, args.baud, timeout=max(0.05, args.ack_timeout_ms / 1000.0))


def send_serial_command(serial_obj, command: ControlCommand, seq: int, ack_timeout_ms: int, ack_mode: str) -> bool:
    line = format_serial_command(command.throttle, command.steering, seq)
    serial_obj.write((line + "\n").encode("utf-8"))
    serial_obj.flush()
    deadline = time.monotonic() + ack_timeout_ms / 1000.0
    while time.monotonic() < deadline:
        raw = serial_obj.readline()
        if not raw:
            continue
        text = raw.decode("utf-8", errors="ignore").strip()
        if parse_ack(text, seq, ack_mode=ack_mode):
            return True
        if text.upper().startswith("NACK"):
            return False
    return False


def load_model(model_dir: str | Path, device: str = "auto"):
    try:
        import torch
        from torch import nn
    except ModuleNotFoundError as exc:
        raise RuntimeError("需要安装训练依赖：pip install torch numpy") from exc

    checkpoint = torch.load(Path(model_dir) / "model.pt", map_location="cpu", weights_only=False)
    config = checkpoint.get("config", {})
    model_config = DriverGRUConfig(
        input_size=int(config["input_size"]),
        hidden_size=int(config.get("hidden_size", 64)),
        num_layers=int(config.get("num_layers", 1)),
        label_columns=list(config.get("label_columns", ["thr", "str"])),
        feature_columns=list(config.get("feature_columns", [])),
    )

    class DriverGRU(nn.Module):
        def __init__(self, cfg: DriverGRUConfig):
            super().__init__()
            self.gru = nn.GRU(cfg.input_size, cfg.hidden_size, cfg.num_layers, batch_first=True)
            self.head = nn.Sequential(nn.Linear(cfg.hidden_size, cfg.hidden_size), nn.ReLU(), nn.Linear(cfg.hidden_size, 2), nn.Tanh())

        def forward(self, xb):
            out, _ = self.gru(xb)
            return self.head(out[:, -1, :])

    model = DriverGRU(model_config)
    model.load_state_dict(checkpoint["model_state_dict"])
    resolved_device = torch.device("cuda" if device == "auto" and torch.cuda.is_available() else ("cpu" if device == "auto" else device))
    model.to(resolved_device)
    model.eval()
    return model, model_config, resolved_device


def predict_control(model, window: Sequence[Sequence[float]], device) -> tuple[float, float]:
    import torch

    tensor = torch.tensor([window], dtype=torch.float32, device=device)
    with torch.no_grad():
        output = model(tensor).cpu().numpy()[0] * 100.0
    return float(output[0]), float(output[1])


def write_log(log_file: Path | None, payload: dict) -> None:
    if not log_file:
        return
    log_file.parent.mkdir(parents=True, exist_ok=True)
    with log_file.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(payload, ensure_ascii=False) + "\n")


def run_loop(args) -> int:
    validate_runtime_mode(args)
    stats = load_standardization(args.model_dir)
    model, model_config, device = load_model(args.model_dir, args.device)
    if model_config.feature_columns and model_config.feature_columns != stats["feature_columns"]:
        raise ValueError("model.pt 中的 feature_columns 与 standardization.json 不一致")
    history: deque[list[float]] = deque()
    state = ControlState()
    limits = ControlLimits(args.max_throttle, args.max_steering, args.max_delta_throttle, args.max_delta_steering)
    serial_obj = open_serial_if_needed(args)
    log_file = Path(args.log_file) if args.log_file else None
    period = 1.0 / args.rate_hz
    seq = 1
    consecutive_errors = 0
    started = time.monotonic()
    try:
        while True:
            if args.duration_sec and time.monotonic() - started >= args.duration_sec:
                break
            loop_started = time.monotonic()
            try:
                latest = fetch_latest_data(args.esp32_url, timeout=args.http_timeout)
                window = build_feature_window(latest, stats, history)
                if window is None:
                    print("warming up")
                    time.sleep(period)
                    continue
                prediction = predict_control(model, window, device)
                command = apply_safety_gate(prediction, latest, limits, state)
                command = command_for_mode(args.mode, command, seq)
                ack = True
                if serial_obj is not None:
                    ack = send_serial_command(serial_obj, command, seq, args.ack_timeout_ms, args.ack_mode)
                if not ack:
                    consecutive_errors += 1
                else:
                    consecutive_errors = 0
                state = ControlState(command.throttle, command.steering)
                print(f"seq={seq} mode={latest.get('mode')} park={latest.get('park')} pred=({prediction[0]:.1f},{prediction[1]:.1f}) cmd=({command.throttle},{command.steering}) reason={command.reason} ack={ack}")
                write_log(log_file, {"seq": seq, "latest_seq": latest.get("seq"), "mode": latest.get("mode"), "park": latest.get("park"), "prediction": prediction, "command": command._asdict(), "ack": ack})
                if consecutive_errors >= args.max_consecutive_errors:
                    raise RuntimeError("连续串口 ACK 失败，进入 fail-safe")
                seq += 1
            except Exception as exc:
                consecutive_errors += 1
                print(f"错误: {exc}", file=sys.stderr)
                if serial_obj is not None:
                    for _ in range(3):
                        try:
                            send_serial_command(serial_obj, ControlCommand(0, 0, "fail_safe"), seq, args.ack_timeout_ms, "any")
                        except Exception:
                            pass
                if consecutive_errors >= args.max_consecutive_errors:
                    return 1
            elapsed = time.monotonic() - loop_started
            time.sleep(max(0.0, period - elapsed))
    finally:
        if serial_obj is not None:
            try:
                serial_obj.close()
            except Exception:
                pass
    return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="MUS4 模型 Pilot 推理控制器")
    parser.add_argument("--esp32-url", default=DEFAULT_ESP32_URL)
    parser.add_argument("--model-dir", required=True)
    parser.add_argument("--serial-port")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--rate-hz", type=float, default=10.0)
    parser.add_argument("--duration-sec", type=float, default=0.0)
    parser.add_argument("--mode", choices=["dry-run", "zero-output", "live"], default="dry-run")
    parser.add_argument("--i-understand-risk", action="store_true")
    parser.add_argument("--max-throttle", type=int, default=10)
    parser.add_argument("--max-steering", type=int, default=30)
    parser.add_argument("--max-delta-throttle", type=int, default=5)
    parser.add_argument("--max-delta-steering", type=int, default=10)
    parser.add_argument("--ack-timeout-ms", type=int, default=100)
    parser.add_argument("--ack-mode", choices=["seq", "any"], default="seq")
    parser.add_argument("--max-consecutive-errors", type=int, default=3)
    parser.add_argument("--http-timeout", type=float, default=1.0)
    parser.add_argument("--log-file")
    parser.add_argument("--device", default="auto")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    try:
        return run_loop(args)
    except Exception as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
