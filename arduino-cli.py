#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json

# 强制设置UTF-8编码（解决Windows控制台乱码问题）
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except Exception:
        pass

import argparse
import subprocess
import logging
import time
import platform
import yaml
import shutil
import threading
import itertools
import glob
import serial
import shlex
import select
import re
from typing import Optional
from serial.tools import list_ports

# 配置日志
class CustomFormatter(logging.Formatter):
    """自定义日志格式，控制台输出带颜色"""
    grey = "\x1b[38;20m"
    yellow = "\x1b[33;20m"
    red = "\x1b[31;20m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"
    format_str = "%(asctime)s - %(levelname)s - %(message)s"

    FORMATS = {
        logging.DEBUG: grey + format_str + reset,
        logging.INFO: grey + format_str + reset,
        logging.WARNING: yellow + format_str + reset,
        logging.ERROR: red + format_str + reset,
        logging.CRITICAL: bold_red + format_str + reset
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt, datefmt='%Y-%m-%d %H:%M:%S')
        return formatter.format(record)

def setup_logging(log_file, level_name):
    level = getattr(logging, level_name.upper(), logging.INFO)
    
    # 确保日志目录存在
    log_dir = os.path.dirname(log_file)
    if log_dir and not os.path.exists(log_dir):
        os.makedirs(log_dir)

    # 文件处理器
    file_handler = logging.FileHandler(log_file)
    file_handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    
    # 控制台处理器
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(CustomFormatter())

    logging.basicConfig(level=level, handlers=[file_handler, console_handler])
    return logging.getLogger("ArduinoCLI")

class Spinner:
    """命令行加载动画，支持动态更新后缀文本（用于进度条等）"""
    def __init__(self, message="Processing... ", delay=0.15, enable_progress=True):
        self.spinner = itertools.cycle(['|', '/', '-', '\\'])
        self.delay = delay
        self.busy = False
        self.message = message
        self.suffix = ""
        # 不严格要求 isatty()，兼容 PowerShell 等环境
        self.enable_progress = enable_progress
        if not sys.stdout.isatty() and os.environ.get("TERM") == "dumb":
            self.enable_progress = False
        self._screen_lock = threading.Lock()
        if not self.enable_progress:
            # 非交互环境或关闭进度时，直接打印消息
            sys.stdout.write(message + "\n")
            sys.stdout.flush()

    def update_suffix(self, text):
        """更新后缀显示文本（如进度条）"""
        with self._screen_lock:
            self.suffix = text
            if self.enable_progress and self.busy:
                # 有进度条时立即渲染，避免卡顿；有进度条时后台线程会停止定时刷新，避免双重渲染
                self._render()

    def _render(self):
        """渲染当前状态到终端（使用 \r 整行覆盖）"""
        if not self.enable_progress:
            return
        # 有进度条时完全禁用旋转字符，仅保留进度条的推进作为动效，避免闪烁
        if self.suffix:
            line = f"\r  {self.message} {self.suffix}"
        else:
            spinner_char = next(self.spinner)
            line = f"\r{spinner_char} {self.message}"
        # 清除行尾残留字符
        line += "\033[K"
        sys.stdout.write(line)
        sys.stdout.flush()

    def spinner_task(self):
        while self.busy:
            with self._screen_lock:
                # 有进度条时跳过定时重绘，完全由 update_suffix 驱动刷新，避免双重渲染导致的闪烁
                if not self.suffix:
                    self._render()
            time.sleep(self.delay)

    def __enter__(self):
        self.busy = True
        if self.enable_progress:
            threading.Thread(target=self.spinner_task, daemon=True).start()
        return self

    def __exit__(self, exception, value, tb):
        self.busy = False
        time.sleep(self.delay)
        with self._screen_lock:
            if self.enable_progress:
                # 清除进度行，显示最终结果
                line = f"\r✅ {self.message}"
                if exception:
                    line = f"\r❌ {self.message}"
                line += "\033[K\n"
                sys.stdout.write(line)
            else:
                if exception:
                    sys.stdout.write('FAILED\n')
                else:
                    sys.stdout.write('DONE\n')
            sys.stdout.flush()


def _espota_version_key(path):
    version = os.path.basename(os.path.dirname(os.path.dirname(path)))
    parts = re.findall(r'\d+|[A-Za-z]+', version)
    key = []
    for part in parts:
        if part.isdigit():
            key.append((1, int(part)))
        else:
            key.append((0, part.lower()))
    return key


def find_espota_tool(explicit_path=None, env=None, home=None, local_appdata=None):
    env = env if env is not None else os.environ
    candidates = []

    if explicit_path:
        return explicit_path if os.path.exists(explicit_path) else None

    env_path = env.get("ESPOTA_PY", "") if env else ""
    if env_path:
        return env_path if os.path.exists(env_path) else None

    if local_appdata is None:
        local_appdata = env.get("LOCALAPPDATA", "") if env else ""
    if local_appdata:
        pattern = os.path.join(local_appdata, "Arduino15", "packages", "esp32", "hardware", "esp32", "*", "tools", "espota.py")
        candidates.extend(glob.glob(pattern))

    if home is None:
        home = os.path.expanduser("~")
    if home:
        pattern = os.path.join(home, ".arduino15", "packages", "esp32", "hardware", "esp32", "*", "tools", "espota.py")
        candidates.extend(glob.glob(pattern))

    candidates = [path for path in candidates if os.path.exists(path)]
    if not candidates:
        return None
    return sorted(candidates, key=_espota_version_key, reverse=True)[0]


def build_espota_command(python_exe, espota_tool, host, port, password, bin_path):
    return [
        python_exe,
        espota_tool,
        "-i", host,
        "-p", str(port),
        "-a", password,
        "-f", bin_path,
    ]


def parse_progress_line(line: str) -> Optional[str]:
    """
    解析上传过程中的固件写入进度行，返回格式化后的进度字符串
    无法识别或非写入阶段则返回 None

    核心识别信号：包含百分比数字（兼容方括号进度条、Writing at 原始格式）
    黑名单：仅过滤擦除、校验、压缩等其他阶段的进度，避免闪烁
    """
    if not line:
        return None

    # 黑名单：仅过滤真正的非写入阶段关键字（写入相关的 Writing / Wrote 等不过滤）
    exclude_keywords = ("Eras", "Verif", "Hash", "Compress", "Check", "CRC", "Leaving", "Reset")
    for kw in exclude_keywords:
        if kw in line:
            return None

    percent = None

    # 匹配模式1: 带小数点的百分比格式，如 " 15.2%"（esptool 格式）
    match = re.search(r'(\d+\.\d+)\s*%', line)
    if match:
        try:
            percent = float(match.group(1))
            percent = max(0.0, min(100.0, percent))
        except (ValueError, IndexError):
            pass

    # 匹配模式2: 整数百分比，如 "(5 %)"、"50%"
    if percent is None:
        match = re.search(r'(\d+)\s*%', line)
        if match:
            try:
                percent = float(match.group(1))
                percent = max(0.0, min(100.0, percent))
            except (ValueError, IndexError):
                pass

    if percent is not None:
        # 统一生成标准格式的进度条
        bar_length = 20
        filled = int(percent / 100 * bar_length)
        bar = "[" + "=" * filled + " " * (bar_length - filled) + "]"
        return f"{bar} {percent:.1f}%"

    return None


class ArduinoAutomation:
    DEFAULT_DESCRIPTION_KEYWORDS = [
        "esp32",
        "usb serial",
        "usb-serial",
        "uart",
        "cp210",
        "ch340",
        "wch",
        "ftdi"
    ]
    DEFAULT_MANUFACTURER_KEYWORDS = [
        "espressif",
        "silicon labs",
        "wch",
        "ftdi"
    ]
    DEFAULT_HWID_KEYWORDS = [
        "vid:pid=10c4:ea60",
        "vid:pid=1a86:7523",
        "vid:pid=0403:6001",
        "vid:pid=303a:"
    ]
    DEFAULT_PREFERRED_DESCRIPTION_KEYWORDS = []
    DEFAULT_PREFERRED_MANUFACTURER_KEYWORDS = []
    DEFAULT_PREFERRED_HWID_KEYWORDS = []
    DEFAULT_STATE_FILE = ".arduino_cli_state.json"

    def __init__(self, config_path, args):
        self.logger = logging.getLogger("ArduinoCLI")
        self.config = self.load_config(config_path)
        self.config_path = config_path
        self.args = args
        
        # 参数优先级：命令行 > 配置文件 > 默认值
        self.arduino_cli = args.cli or self.config.get('default', {}).get('arduino_cli', 'ArduinoCLI')
        self.fqbn = args.fqbn or self.config.get('default', {}).get('fqbn', 'esp32:esp32:esp32')
        self.port = args.port or self.config.get('default', {}).get('port', '')
        self.baud = args.baud or self.config.get('default', {}).get('baudrate', 115200)
        self.sketch = args.sketch or self.config.get('default', {}).get('sketch_path', '')
        
        reset_cfg = self.config.get('reset', {})
        self.reset_enabled = args.auto_reset if args.auto_reset is not None else reset_cfg.get('enable', True)
        self.reset_delay_ms = args.reset_delay or reset_cfg.get('delay_ms', 200)
        self.reset_method = (args.reset or reset_cfg.get('method') or 'dtr_rts').lower()
        self.reset_boot = (args.reset_boot or reset_cfg.get('boot') or 'run').lower()
        self.reset_toolchain = (args.reset_toolchain or reset_cfg.get('toolchain') or '').lower()
        self.reset_command = args.reset_command or reset_cfg.get('command', '')
        self.serial_detection_cfg = self.config.get('serial_detection', {}) or {}
        self.serial_detection_enabled = self.serial_detection_cfg.get('enabled', True)
        self.serial_state_file = self._resolve_serial_state_file()

        # 转换 sketch 路径为绝对路径
        if not os.path.isabs(self.sketch):
            base_dir = os.path.dirname(os.path.abspath(config_path))
            self.sketch = os.path.join(base_dir, self.sketch)
            
        # 检测操作系统
        self.os_type = platform.system()
        self.validate_environment()
        self.log_reset_interfaces()

    def _resolve_serial_state_file(self):
        configured = self.serial_detection_cfg.get("state_file", self.DEFAULT_STATE_FILE)
        state_file = (configured or self.DEFAULT_STATE_FILE).strip()
        if not os.path.isabs(state_file):
            base_dir = os.path.dirname(os.path.abspath(self.config_path))
            state_file = os.path.join(base_dir, state_file)
        return state_file

    def _load_serial_state(self):
        if not os.path.exists(self.serial_state_file):
            return {}
        try:
            with open(self.serial_state_file, "r", encoding="utf-8") as f:
                state = json.load(f)
                if isinstance(state, dict):
                    return state
        except Exception as e:
            self.logger.warning(f"读取串口状态缓存失败，已忽略: {e}")
        return {}

    def get_last_success_port(self):
        state = self._load_serial_state()
        last_port = str(state.get("last_success_port", "")).strip()
        if not last_port:
            return ""
        return last_port

    def save_last_success_port(self, port):
        if not port:
            return
        state_dir = os.path.dirname(self.serial_state_file)
        if state_dir and not os.path.exists(state_dir):
            os.makedirs(state_dir, exist_ok=True)

        state = self._load_serial_state()
        state["last_success_port"] = port
        state["updated_at"] = time.strftime("%Y-%m-%d %H:%M:%S")
        try:
            with open(self.serial_state_file, "w", encoding="utf-8") as f:
                json.dump(state, f, ensure_ascii=False, indent=2)
            self.logger.info(f"已记录本次成功串口: {port}")
        except Exception as e:
            self.logger.warning(f"写入串口状态缓存失败: {e}")

    def load_config(self, path):
        if not os.path.exists(path):
            self.logger.warning(f"配置文件 {path} 不存在，使用默认设置")
            return {}
        try:
            with open(path, 'r', encoding='utf-8') as f:
                return yaml.safe_load(f) or {}
        except Exception as e:
            self.logger.error(f"加载配置文件失败: {e}")
            sys.exit(1)

    def validate_environment(self):
        self.logger.info(f"检测到操作系统: {self.os_type}")
        
        # 检查 ArduinoCLI
        if not shutil.which(self.arduino_cli):
            self.logger.error(f"找不到命令: {self.arduino_cli}")
            sys.exit(2)
            
        # 检查 sketch 文件
        if not os.path.exists(self.sketch):
            self.logger.error(f"找不到 Sketch 文件: {self.sketch}")
            sys.exit(3)

    def log_reset_interfaces(self):
        toolchain = self.reset_toolchain or ("arduino-cli" if self.arduino_cli else "")
        interfaces = {
            "arduino-cli": ["dtr_rts", "1200bps"],
            "openocd": ["monitor reset run", "monitor reset halt"],
            "jlink": ["JLinkReset", "reset", "r"],
            "st-link": ["st-flash reset"],
            "pyocd": ["pyocd reset"],
            "cmsis": ["NVIC_SystemReset", "AIRCR"]
        }
        available = interfaces.get(toolchain, ["custom_command"])
        self.logger.info(f"复位接口识别: toolchain={toolchain or 'unknown'} methods={','.join(available)}")

    def _normalize_text_list(self, values, defaults=None):
        if values is None:
            values = defaults or []
        elif isinstance(values, str):
            values = [values]
        normalized = []
        for value in values:
            text = str(value).strip().lower()
            if text:
                normalized.append(text)
        return normalized

    def _configured_port_mode(self):
        port = (self.port or "").strip()
        if not port:
            return "auto"
        if port.lower() in {"auto", "detect", "scan"}:
            return "auto"
        return "fixed"

    def enumerate_serial_ports(self):
        ports = []
        for port in list_ports.comports():
            ports.append({
                "device": port.device or "",
                "description": port.description or "",
                "manufacturer": getattr(port, "manufacturer", "") or "",
                "product": getattr(port, "product", "") or "",
                "hwid": port.hwid or "",
                "serial_number": getattr(port, "serial_number", "") or "",
                "location": getattr(port, "location", "") or "",
                "vid": getattr(port, "vid", None),
                "pid": getattr(port, "pid", None)
            })
        ports.sort(key=lambda item: item["device"].lower())
        return ports

    def format_port_summary(self, port_info):
        summary = f'{port_info["device"]}: {port_info["description"] or "Unknown"}'
        extras = []
        if port_info.get("manufacturer"):
            extras.append(f'mfr={port_info["manufacturer"]}')
        if port_info.get("product"):
            extras.append(f'product={port_info["product"]}')
        if port_info.get("serial_number"):
            extras.append(f'sn={port_info["serial_number"]}')
        if port_info.get("vid") is not None and port_info.get("pid") is not None:
            extras.append(f'vid_pid={port_info["vid"]:04X}:{port_info["pid"]:04X}')
        if extras:
            summary += f' ({", ".join(extras)})'
        return summary

    def list_available_ports(self, level="info"):
        ports = self.enumerate_serial_ports()
        if not ports:
            getattr(self.logger, level)("未检测到可用串口")
            return ports
        getattr(self.logger, level)(f"检测到 {len(ports)} 个串口设备:")
        for port in ports:
            getattr(self.logger, level)(f"  - {self.format_port_summary(port)}")
        return ports

    def score_port_candidate(self, port_info):
        score = 0
        reasons = []
        description_text = f'{port_info.get("description", "")} {port_info.get("product", "")}'.lower()
        manufacturer_text = port_info.get("manufacturer", "").lower()
        hwid_text = port_info.get("hwid", "").lower()

        description_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("description_keywords"),
            self.DEFAULT_DESCRIPTION_KEYWORDS
        )
        manufacturer_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("manufacturer_keywords"),
            self.DEFAULT_MANUFACTURER_KEYWORDS
        )
        hwid_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("hwid_keywords"),
            self.DEFAULT_HWID_KEYWORDS
        )
        preferred_description_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("preferred_description_keywords"),
            self.DEFAULT_PREFERRED_DESCRIPTION_KEYWORDS
        )
        preferred_manufacturer_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("preferred_manufacturer_keywords"),
            self.DEFAULT_PREFERRED_MANUFACTURER_KEYWORDS
        )
        preferred_hwid_keywords = self._normalize_text_list(
            self.serial_detection_cfg.get("preferred_hwid_keywords"),
            self.DEFAULT_PREFERRED_HWID_KEYWORDS
        )

        for keyword in description_keywords:
            if keyword in description_text:
                score += 30
                reasons.append(f"description:{keyword}")
        for keyword in manufacturer_keywords:
            if keyword in manufacturer_text:
                score += 20
                reasons.append(f"manufacturer:{keyword}")
        for keyword in hwid_keywords:
            if keyword in hwid_text:
                score += 40
                reasons.append(f"hwid:{keyword}")
        for keyword in preferred_description_keywords:
            if keyword in description_text:
                score += 120
                reasons.append(f"preferred_description:{keyword}")
        for keyword in preferred_manufacturer_keywords:
            if keyword in manufacturer_text:
                score += 100
                reasons.append(f"preferred_manufacturer:{keyword}")
        for keyword in preferred_hwid_keywords:
            if keyword in hwid_text:
                score += 140
                reasons.append(f"preferred_hwid:{keyword}")

        vid = port_info.get("vid")
        pid = port_info.get("pid")
        vid_pid = None
        if vid is not None and pid is not None:
            vid_pid = f"{vid:04x}:{pid:04x}"

        configured_vid_pid = self._normalize_text_list(self.serial_detection_cfg.get("vid_pid"))
        if vid_pid and configured_vid_pid and vid_pid in configured_vid_pid:
            score += 60
            reasons.append(f"vid_pid:{vid_pid}")

        preferred_vid_pid = self._normalize_text_list(self.serial_detection_cfg.get("preferred_vid_pid"))
        if vid_pid and preferred_vid_pid and vid_pid in preferred_vid_pid:
            score += 160
            reasons.append(f"preferred_vid_pid:{vid_pid}")

        return score, reasons

    def select_best_port(self, ports):
        if not ports:
            return None, "未检测到可用串口"

        fixed_port = (self.port or "").strip()
        if self._configured_port_mode() == "fixed":
            for port in ports:
                if port["device"].lower() == fixed_port.lower():
                    return port, f"命中指定端口 {fixed_port}"
            if not self.serial_detection_enabled:
                return None, f"指定端口 {fixed_port} 未连接，且已禁用自动匹配"
            self.logger.warning(f"指定端口 {fixed_port} 未连接，尝试自动匹配")

        scored = []
        for port in ports:
            score, reasons = self.score_port_candidate(port)
            if score > 0:
                scored.append((score, reasons, port))

        scored.sort(key=lambda item: (-item[0], item[2]["device"].lower()))

        if len(scored) == 1:
            score, reasons, port = scored[0]
            return port, f"自动匹配命中 {port['device']} (score={score}, {', '.join(reasons)})"

        if len(scored) > 1:
            top_score = scored[0][0]
            best = [item for item in scored if item[0] == top_score]
            if len(best) == 1:
                score, reasons, port = best[0]
                return port, f"自动匹配命中 {port['device']} (score={score}, {', '.join(reasons)})"
            devices = ", ".join(item[2]["device"] for item in best)
            return None, f"检测到多个同分候选串口: {devices}，请显式指定端口"

        if len(ports) == 1:
            port = ports[0]
            return port, f"仅检测到一个串口，安全兜底选择 {port['device']}"

        return None, "未找到满足匹配规则的串口，请指定 --port 或调整 config.yaml 的 serial_detection 配置"

    def get_port_identity(self, port_info):
        serial_number = (port_info.get("serial_number") or "").strip().lower()
        if serial_number:
            return ("serial_number", serial_number)

        location = (port_info.get("location") or "").strip().lower()
        if location:
            return ("location", location)

        vid = port_info.get("vid")
        pid = port_info.get("pid")
        manufacturer = (port_info.get("manufacturer") or "").strip().lower()
        if vid is not None and pid is not None and manufacturer:
            return ("bridge", f"{vid:04x}:{pid:04x}:{manufacturer}")

        return None

    def build_upload_port_attempts(self):
        ports = self.enumerate_serial_ports()
        if not ports:
            return [], "未检测到可用串口"

        # 显式指定命令行端口时，尊重用户意图，不自动切换其他端口。
        if getattr(self.args, "port", None):
            primary, reason = self.select_best_port(ports)
            if not primary:
                return [], reason or "未指定上传端口或自动匹配失败"
            attempts = [{
                "port": primary,
                "reason": reason,
            }]
            return attempts, None

        attempts = []
        seen_ports = set()
        port_lookup = {p["device"].lower(): p for p in ports}

        # 优先尝试上一次成功的串口，避免每次都重新猜测。
        remembered_port = self.get_last_success_port()
        if remembered_port:
            remembered = port_lookup.get(remembered_port.lower())
            if remembered:
                attempts.append({
                    "port": remembered,
                    "reason": "命中上一次成功上传端口"
                })
                seen_ports.add(remembered["device"].lower())
            else:
                self.logger.warning(f"上一次成功串口 {remembered_port} 当前不可用，自动切换到候选端口")

        primary, reason = self.select_best_port(ports)
        if not primary and not attempts:
            return [], reason or "未指定上传端口或自动匹配失败"
        if primary and primary["device"].lower() not in seen_ports:
            attempts.append({
                "port": primary,
                "reason": reason,
            })
            seen_ports.add(primary["device"].lower())

        identity_source = attempts[0]["port"] if attempts else primary
        if not identity_source:
            return [], reason or "未指定上传端口或自动匹配失败"

        primary_identity = self.get_port_identity(identity_source)
        if not primary_identity:
            return attempts, None

        for port in ports:
            normalized = port["device"].lower()
            if normalized in seen_ports:
                continue
            if self.get_port_identity(port) == primary_identity:
                attempts.append({
                    "port": port,
                    "reason": f"备用候选：与 {identity_source['device']} 属于同一 USB 串口设备"
                })
                seen_ports.add(normalized)

        return attempts, None

    def resolve_port(self, required=False):
        ports = self.enumerate_serial_ports()

        # 对于自动模式（且未显式传 --port），优先使用上次成功串口。
        if not getattr(self.args, "port", None) and self._configured_port_mode() == "auto":
            remembered_port = self.get_last_success_port()
            if remembered_port:
                for port in ports:
                    if port["device"].lower() == remembered_port.lower():
                        self.port = port["device"]
                        self.logger.info(f"串口解析结果: {self.port} (命中上一次成功串口)")
                        return self.port
                self.logger.warning(f"上一次成功串口 {remembered_port} 当前不可用，改为自动匹配")

        selected, reason = self.select_best_port(ports)
        if selected:
            resolved_port = selected["device"]
            if resolved_port != self.port:
                self.logger.info(f"串口解析结果: {resolved_port} ({reason})")
            self.port = resolved_port
            return resolved_port

        if reason:
            self.logger.error(reason)
        self.list_available_ports(level="error")
        if required:
            return None
        return self.port

    def detach_console_handlers(self):
        root_logger = logging.getLogger()
        removed = []
        for handler in list(root_logger.handlers):
            if isinstance(handler, logging.StreamHandler) and handler.stream in (sys.stdout, sys.stderr):
                removed.append(handler)
                root_logger.removeHandler(handler)
        return removed

    def restore_console_handlers(self, handlers):
        root_logger = logging.getLogger()
        for handler in handlers:
            if handler not in root_logger.handlers:
                root_logger.addHandler(handler)

    def run_command(self, cmd, timeout=None, message="Processing... ", enable_progress=True):
        self.logger.debug(f"执行命令: {' '.join(cmd)}")
        start_time = time.time()

        # 判断是否启用进度条：非verbose + 启用进度 + 没传--no-progress
        # 注意：不严格要求 isatty()，因为 PowerShell 等环境下 isatty() 会返回 False 但实际支持 ANSI
        use_progress = enable_progress
        use_progress = use_progress and self.logger.getEffectiveLevel() >= logging.INFO
        use_progress = use_progress and not (hasattr(self.args, 'no_progress') and self.args.no_progress)
        # 如果显式传了 --no-progress 或 stdout 明显是文件/管道，再关闭
        if not sys.stdout.isatty() and os.environ.get("TERM") == "dumb":
            use_progress = False

        try:
            spinner = None
            if self.logger.getEffectiveLevel() >= logging.INFO:
                spinner = Spinner(message, delay=0.15, enable_progress=use_progress)
                spinner.__enter__()

            # 使用 Popen 逐行读取输出，以便实时解析进度
            # esptool/arduino-cli 会把进度输出到 stderr，所以必须捕获 stderr
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,  # 合并 stdout 和 stderr，保证输出顺序
                text=True,
                encoding='utf-8',
                errors='replace',
                bufsize=1,
                universal_newlines=True
            )

            stdout_lines = []
            # 只显示最后一个（最大的）固件写入阶段的进度，前面的 bootloader/分区表等小阶段静默
            # ESP32 app 固件起始地址为 0x10000，是最后且最大的写入段
            show_progress = False
            LAST_STAGE_ADDR = 0x10000  # ESP32 app 固件起始地址

            for line in process.stdout:
                line = line.rstrip('\n')
                stdout_lines.append(line)

                if not line:
                    continue

                # 阶段识别：检测到 Writing at 0x... 行时判断是否是最后一个大段
                if use_progress and "Writing at 0x" in line:
                    match = re.search(r'Writing at 0x([0-9a-fA-F]+)', line)
                    if match:
                        try:
                            addr = int(match.group(1), 16)
                            show_progress = addr >= LAST_STAGE_ADDR
                        except ValueError:
                            pass

                # 尝试解析为进度行
                progress_text = None
                if use_progress and show_progress:
                    progress_text = parse_progress_line(line)

                if progress_text and spinner:
                    # 是进度行，更新 spinner 后缀，spinner 自己负责渲染，不清行
                    spinner.update_suffix(progress_text)
                else:
                    # 普通输出行：不再触发清行，避免进度条"闪没"
                    # 普通中间行（Writing at 0x... 等）静默吞掉，不干扰进度条显示
                    is_important = "error" in line.lower() or "fail" in line.lower() or "warning" in line.lower()
                    if self.logger.getEffectiveLevel() <= logging.DEBUG:
                        # debug 模式打印全部输出
                        sys.stdout.write("\r\033[K" + line + "\n")
                        if spinner:
                            spinner._render()
                    elif is_important:
                        # 重要信息：清行打印，打印后立即重绘进度条，避免进度条消失
                        sys.stdout.write("\r\033[K" + line + "\n")
                        if spinner:
                            spinner._render()
                    sys.stdout.flush()

            # 等待进程结束
            returncode = process.wait(timeout=timeout)
            stdout_full = "\n".join(stdout_lines)

            if returncode != 0:
                raise subprocess.CalledProcessError(returncode, cmd, output=stdout_full, stderr="")

            if spinner:
                spinner.__exit__(None, None, None)

            duration = time.time() - start_time
            self.logger.info(f"命令执行成功 (耗时 {duration:.2f}s)")
            self.logger.debug(f"输出:\n{stdout_full}")
            return True, stdout_full

        except subprocess.CalledProcessError as e:
            if spinner:
                spinner.__exit__(type(e), e, None)
            self.logger.error(f"命令执行失败 (退出码 {e.returncode})")
            self.logger.error(f"错误输出:\n{e.stdout}")
            return False, e.stdout
        except subprocess.TimeoutExpired:
            process.kill()
            if spinner:
                spinner.__exit__(type(e), e, None)
            self.logger.error(f"命令执行超时 ({timeout}s)")
            return False, "Timeout"
        except Exception as e:
            if spinner:
                spinner.__exit__(type(e), e, None)
            self.logger.error(f"未知错误: {e}")
            return False, str(e)

    def compile(self):
        self.logger.info(f"开始编译: {os.path.basename(self.sketch)} ({self.fqbn})")
        cmd = [self.arduino_cli, "compile", "--fqbn", self.fqbn]
        
        # 支持指定构建输出目录
        build_path = None
        if self.args and getattr(self.args, 'build_path', None):
            build_path = self.args.build_path
        else:
            # 检查配置文件中的build_path
            build_path = self.config.get('default', {}).get('build_path')
        
        if build_path:
            # 转换为绝对路径
            if not os.path.isabs(build_path):
                base_dir = os.path.dirname(os.path.abspath(self.args.config if self.args else 'config.yaml'))
                build_path = os.path.join(base_dir, build_path)
            cmd.extend(["--build-path", build_path, "--output-dir", build_path])
            self.logger.info(f"构建输出目录: {build_path}")
        
        cmd.append(self.sketch)
        success, _ = self.run_command(cmd, message="正在编译... ")
        if not success:
            self.logger.error("编译失败，终止流程")
            sys.exit(10)
        return True

    def normalize_precompiled_input_file(self, input_file):
        lower = os.path.basename(input_file).lower()
        suffixes = (".bootloader.bin", ".partitions.bin", ".merged.bin")
        if not lower.endswith(suffixes):
            return input_file

        directory = os.path.dirname(input_file) or "."
        stem = os.path.basename(input_file)
        for suffix in (".bootloader.bin", ".partitions.bin", ".merged.bin"):
            if stem.lower().endswith(suffix):
                candidate = os.path.join(directory, stem[:-len(suffix)] + ".bin")
                if os.path.exists(candidate):
                    self.logger.warning(f"指定文件是烧录分片，自动改用主固件: {candidate}")
                    return candidate
        return input_file

    def build_upload_command(self, port):
        if self.args and getattr(self.args, 'input_file', None):
            input_file = self.normalize_precompiled_input_file(self.args.input_file)
            if not os.path.exists(input_file):
                self.logger.error(f"指定的固件文件不存在: {input_file}")
                sys.exit(14)
            self.logger.info(f"使用预编译固件: {input_file}")
            return [self.arduino_cli, "upload", "-p", port, "--fqbn", self.fqbn,
                    "--input-file", input_file, self.sketch]

        build_path = None
        if self.args and getattr(self.args, 'build_path', None):
            build_path = self.args.build_path
        else:
            build_path = self.config.get('default', {}).get('build_path')

        cmd = [self.arduino_cli, "upload", "-p", port, "--fqbn", self.fqbn]

        if build_path:
            if not os.path.isabs(build_path):
                base_dir = os.path.dirname(os.path.abspath(self.args.config if self.args else 'config.yaml'))
                build_path = os.path.join(base_dir, build_path)
            cmd.extend(["--input-dir", build_path])

        cmd.append(self.sketch)
        return cmd

    def ota_upload(self):
        if not self.args or not getattr(self.args, 'input_file', None):
            self.logger.error("OTA 上传需要通过 --input-file 指定固件 .bin 文件")
            sys.exit(15)

        input_file = self.normalize_precompiled_input_file(self.args.input_file)
        if not os.path.exists(input_file):
            self.logger.error(f"指定的固件文件不存在: {input_file}")
            sys.exit(14)

        host = getattr(self.args, 'ota_host', None)
        if not host:
            self.logger.error("OTA 上传需要指定 --ota-host")
            sys.exit(15)

        port = getattr(self.args, 'ota_port', None) or 3232
        password = getattr(self.args, 'ota_password', None) or ""
        espota_tool = find_espota_tool(getattr(self.args, 'espota_tool', None))
        if not espota_tool:
            self.logger.error("找不到 espota.py，请使用 --espota-tool 指定路径或设置 ESPOTA_PY")
            sys.exit(15)

        cmd = build_espota_command(sys.executable, espota_tool, host, port, password, input_file)
        success, output = self.run_command(cmd, message="正在 OTA 上传... ")
        if success:
            return True

        if output:
            self.logger.error(f"OTA 上传失败:\n{output}")
        sys.exit(12)

    def upload(self):
        port_attempts, error = self.build_upload_port_attempts()
        if not port_attempts:
            self.logger.error(error or "未指定上传端口或自动匹配失败")
            self.list_available_ports(level="error")
            sys.exit(11)

        last_error = ""
        for index, attempt in enumerate(port_attempts):
            port_info = attempt["port"]
            self.port = port_info["device"]

            if self.os_type != "Windows" and not os.path.exists(self.port):
                self.logger.warning(f"端口 {self.port} 未在文件系统中检测到，尝试继续...")

            if index == 0:
                self.logger.info(f"开始上传到端口: {self.port}")
            else:
                self.logger.warning(f"上传失败后自动切换到 {self.port} 重试 ({attempt['reason']})")
                time.sleep(0.3)

            cmd = self.build_upload_command(self.port)
            success, output = self.run_command(cmd, message="正在上传... ")
            if success:
                self.save_last_success_port(self.port)
                return True

            last_error = output

        if last_error:
            self.logger.error(f"所有候选串口均上传失败，最后一次错误:\n{last_error}")
        self.logger.error("上传失败，终止流程")
        sys.exit(12)

    def dtr_rts_reset(self, boot_mode):
        ser = serial.Serial(
            port=self.port,
            baudrate=self.baud,
            timeout=1
        )
        ser.dtr = False
        ser.rts = False
        time.sleep(0.05)
        if boot_mode == "flash":
            ser.dtr = True
            ser.rts = True
            time.sleep(0.05)
            ser.rts = False
            time.sleep(0.05)
        else:
            ser.rts = True
            time.sleep(0.05)
            ser.rts = False
            time.sleep(0.05)
        ser.dtr = False
        ser.close()

    def baud_toggle_reset(self):
        ser = serial.Serial(
            port=self.port,
            baudrate=1200,
            timeout=1
        )
        ser.close()
        time.sleep(0.2)

    def command_reset(self):
        if not self.reset_command:
            raise RuntimeError("reset.command 未配置")
        cmd = shlex.split(self.reset_command)
        success, _ = self.run_command(cmd, message="正在复位... ")
        return success

    def auto_reset(self):
        if not self.reset_enabled:
            return False
        if self.reset_method in ["dtr_rts", "1200bps"] and not self.resolve_port(required=True):
            self.logger.warning("自动复位跳过：未指定串口端口")
            return False
        self.logger.info(f"正在自动复位单片机: {self.port}")
        start_time = time.time()
        time.sleep(self.reset_delay_ms / 1000.0)
        try:
            if self.reset_method == "dtr_rts":
                self.dtr_rts_reset(self.reset_boot)
            elif self.reset_method == "1200bps":
                self.baud_toggle_reset()
            elif self.reset_method == "command":
                success = self.command_reset()
                if not success:
                    raise RuntimeError("复位命令执行失败")
            else:
                raise RuntimeError(f"未知复位方式: {self.reset_method}")
            duration = (time.time() - start_time) * 1000
            self.logger.info(f"Auto-reset triggered ({duration:.1f}ms)")
            return True
        except Exception as e:
            self.logger.warning(f"自动复位失败: {e}")
            self.logger.warning("可能需要手动复位单片机")
            return False

    def _keyboard_listener(self, stop_event):
        """监听键盘输入，检测ESC键"""
        try:
            if self.os_type == "Windows":
                import msvcrt
                while not stop_event.is_set():
                    if msvcrt.kbhit():
                        ch = msvcrt.getch()
                        if ch == b'\x1b':  # ESC key
                            stop_event.set()
                            break
                    time.sleep(0.02)
            else:
                # Linux/Mac: 使用 select
                import tty
                import termios
                old_settings = termios.tcgetattr(sys.stdin)
                try:
                    tty.setcbreak(sys.stdin.fileno())
                    while not stop_event.is_set():
                        if select.select([sys.stdin], [], [], 0.1)[0]:
                            ch = sys.stdin.read(1)
                            if ch == '\x1b':  # ESC key
                                stop_event.set()
                                break
                finally:
                    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        except Exception as e:
            self.logger.debug(f"键盘监听线程异常: {e}")

    def monitor(self):
        if not self.resolve_port(required=True):
            self.logger.error("未指定串口或自动匹配失败")
            sys.exit(13)

        # 打开监控前先尝试自动复位
        self.auto_reset()

        console_handlers = self.detach_console_handlers()
        self.logger.info(f"打开串口监控: {self.port} @ {self.baud}")
        self.logger.info("按 ESC 或 Ctrl+C 退出监控")

        # 清屏以避免 TUI 重叠
        if platform.system() == "Windows":
            os.system("cls")
        else:
            os.system("clear")

        ser = None
        stop_event = threading.Event()
        keyboard_thread = None

        try:
            # 打开串口
            ser = serial.Serial(self.port, self.baud, timeout=0.1)

            # 启动键盘监听线程
            keyboard_thread = threading.Thread(
                target=self._keyboard_listener,
                args=(stop_event,),
                daemon=True
            )
            keyboard_thread.start()

            # 读取并显示串口数据
            while not stop_event.is_set():
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    try:
                        # 尝试UTF-8解码，失败则替换乱码
                        text = data.decode('utf-8', errors='replace')
                        sys.stdout.write(text)
                        sys.stdout.flush()
                    except Exception:
                        pass
                time.sleep(0.005)

        except KeyboardInterrupt:
            pass
        except serial.SerialException as e:
            self.logger.error(f"串口错误: {e}")
        except Exception as e:
            self.logger.error(f"监控异常: {e}")
        finally:
            stop_event.set()
            if keyboard_thread and keyboard_thread.is_alive():
                keyboard_thread.join(timeout=0.5)
            if ser and ser.is_open:
                ser.close()
            # 恢复光标显示并清屏
            if self.os_type == "Windows":
                os.system("cls")
            else:
                os.system("clear")
            sys.stdout.write("\033[?25h")
            sys.stdout.flush()
            self.restore_console_handlers(console_handlers)

        self.logger.info("用户停止监控")

    def run(self):
        total_start = time.time()

        if self.args.list_ports:
            self.list_available_ports()
            if not (self.args.compile or self.args.upload or self.args.serial or self.args.regress_reset or self.args.ota):
                return

        # 如果没有指定任何操作，默认显示帮助
        if not (self.args.compile or self.args.upload or self.args.serial or self.args.regress_reset or self.args.ota):
            self.logger.warning("未指定任何操作。请使用 -c, -u, -s, --ota 参数。")
            return

        # 1. 编译
        if self.args.compile:
            self.compile()

        # 2. 上传 (如果只指定上传，也会执行；如果指定了编译+上传，编译失败会终止)
        if self.args.upload or self.args.ota:
            if self.args.ota:
                self.ota_upload()
            else:
                self.upload()
                self.auto_reset()

        # 3. 监控
        if self.args.serial:
            # 简单的延时确保串口已就绪
            if self.args.upload:
                time.sleep(1)
            self.monitor()

        if self.args.regress_reset:
            self.regress_reset(self.args.regress_count)
            
        total_duration = time.time() - total_start
        self.logger.info(f"所有任务完成，总耗时: {total_duration:.2f}s")

    def regress_reset(self, count):
        if count <= 0:
            self.logger.warning("回归测试次数无效，跳过")
            return
        success_count = 0
        durations = []
        self.logger.info(f"开始自动复位回归测试: {count} 次")
        for i in range(count):
            self.logger.info(f"回归测试轮次: {i + 1}/{count}")
            self.upload()
            start = time.time()
            ok = self.auto_reset()
            dur = (time.time() - start) * 1000
            durations.append(dur)
            if ok:
                success_count += 1
        success_rate = (success_count / count) * 100
        avg_extra = sum(durations) / len(durations)
        self.logger.info(f"回归结果: success_rate={success_rate:.2f}% avg_extra={avg_extra:.1f}ms")
        if success_rate < 99.0 or avg_extra >= 300.0:
            self.reset_enabled = False
            self.logger.warning("自动复位达标失败，已回退到手动模式")

def setup_signal_handlers():
    """设置信号处理器，确保中断时恢复终端状态"""
    def handle_sigint(signum, frame):
        # 恢复光标和终端状态
        sys.stdout.write("\033[?25h\n")
        sys.stdout.flush()
        sys.exit(1)

    try:
        import signal
        signal.signal(signal.SIGINT, handle_sigint)
        signal.signal(signal.SIGTERM, handle_sigint)
    except (ValueError, ImportError):
        # Windows 下部分信号不支持，忽略
        pass


def main():
    setup_signal_handlers()
    parser = argparse.ArgumentParser(description="Arduino 项目自动化构建脚本")
    
    # 操作标志
    parser.add_argument('-c', '--compile', action='store_true', help='执行编译')
    parser.add_argument('-u', '--upload', action='store_true', help='执行上传')
    parser.add_argument('-s', '--serial', action='store_true', help='打开串口监控')
    
    # 配置参数
    parser.add_argument('--port', '-p', help='串口设备路径 (e.g., /dev/ttyACM0, COM3)')
    parser.add_argument('--baud', '-b', type=int, help='串口波特率')
    parser.add_argument('--fqbn', help='板型定义 (FQBN)')
    parser.add_argument('--sketch', help='Arduino Sketch 文件路径')
    parser.add_argument('--cli', help='ArduinoCLI 可执行文件路径')
    parser.add_argument('--config', default='config.yaml', help='配置文件路径')
    parser.add_argument('--auto-reset', dest='auto_reset', action='store_true', help='启用自动复位')
    parser.add_argument('--no-auto-reset', dest='auto_reset', action='store_false', help='禁用自动复位')
    parser.add_argument('--reset', dest='reset', help='复位方式: dtr_rts|1200bps|command')
    parser.add_argument('--reset-delay', dest='reset_delay', type=int, help='复位前延时毫秒')
    parser.add_argument('--reset-boot', dest='reset_boot', help='复位模式: run|flash')
    parser.add_argument('--reset-command', dest='reset_command', help='自定义复位命令')
    parser.add_argument('--reset-toolchain', dest='reset_toolchain', help='烧录工具链标识')
    parser.add_argument('--regress-reset', dest='regress_reset', action='store_true', help='自动复位回归测试')
    parser.add_argument('--regress-count', dest='regress_count', type=int, default=10, help='回归测试次数')
    parser.add_argument('--input-file', '-i', dest='input_file', help='指定预编译的固件文件(.bin)路径，用于WSL交叉编译场景')
    parser.add_argument('--build-path', dest='build_path', help='指定构建输出目录(用于编译时指定输出位置)')
    parser.add_argument('--ota', dest='ota', action='store_true', help='使用 ArduinoOTA 通过网络上传固件')
    parser.add_argument('--ota-host', dest='ota_host', help='ArduinoOTA 目标主机或 IP')
    parser.add_argument('--ota-port', dest='ota_port', type=int, default=3232, help='ArduinoOTA 目标端口')
    parser.add_argument('--ota-password', dest='ota_password', default='mus4-debug', help='ArduinoOTA 密码')
    parser.add_argument('--espota-tool', dest='espota_tool', help='espota.py 工具路径')
    parser.add_argument('--list-ports', dest='list_ports', action='store_true', help='列出当前检测到的串口设备')
    parser.add_argument('--no-progress', dest='no_progress', action='store_true', help='关闭单行进度条刷新，逐行输出所有日志')
    parser.set_defaults(auto_reset=None)
    
    args = parser.parse_args()
    
    # 确定配置文件绝对路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = args.config if os.path.isabs(args.config) else os.path.join(script_dir, args.config)
    
    # 初始化日志 (先加载配置以获取日志路径)
    # 这里为了简化，先读取一次配置或使用默认
    log_file = os.path.join(script_dir, "ArduinoCLI.log")
    try:
        with open(config_path, 'r') as f:
            cfg = yaml.safe_load(f)
            if cfg and 'logging' in cfg:
                log_file = cfg['logging'].get('file', log_file)
                if not os.path.isabs(log_file):
                    log_file = os.path.join(script_dir, log_file)
    except:
        pass
        
    logger = setup_logging(log_file, "INFO")
    
    ArduinoCLI = ArduinoAutomation(config_path, args)
    ArduinoCLI.run()

if __name__ == "__main__":
    main()
