#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys

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
import serial
import shlex
import select

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
    """命令行加载动画"""
    def __init__(self, message="Processing... ", delay=0.15):
        self.spinner = itertools.cycle(['⣾', '⣽', '⣻', '⢿', '⡿', '⣟', '⣯', '⣷'])
        self.delay = delay
        self.busy = False
        self._screen_lock = threading.Lock()
        sys.stdout.write(message)
        sys.stdout.flush()

    def spinner_task(self):
        while self.busy:
            with self._screen_lock:
                sys.stdout.write(next(self.spinner))
                sys.stdout.flush()
            time.sleep(self.delay)
            with self._screen_lock:
                sys.stdout.write('\b')
                sys.stdout.flush()

    def __enter__(self):
        self.busy = True
        threading.Thread(target=self.spinner_task).start()

    def __exit__(self, exception, value, tb):
        self.busy = False
        time.sleep(self.delay)
        with self._screen_lock:
            if exception:
                sys.stdout.write('FAILED\n')
            else:
                sys.stdout.write('DONE\n')
            sys.stdout.flush()

class ArduinoAutomation:
    def __init__(self, config_path, args):
        self.logger = logging.getLogger("ArduinoCLI")
        self.config = self.load_config(config_path)
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

        # 转换 sketch 路径为绝对路径
        if not os.path.isabs(self.sketch):
            base_dir = os.path.dirname(os.path.abspath(config_path))
            self.sketch = os.path.join(base_dir, self.sketch)
            
        # 检测操作系统
        self.os_type = platform.system()
        self.validate_environment()
        self.log_reset_interfaces()

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

    def run_command(self, cmd, timeout=None, message="Processing... "):
        self.logger.debug(f"执行命令: {' '.join(cmd)}")
        start_time = time.time()
        
        # Determine if we should use spinner (only if not verbose debugging)
        use_spinner = self.logger.getEffectiveLevel() >= logging.INFO
        
        try:
            if use_spinner:
                spinner = Spinner(message, delay=0.15)
                spinner.busy = True
                t = threading.Thread(target=spinner.spinner_task)
                t.start()
                
            result = subprocess.run(
                cmd, 
                check=True, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE, 
                text=True,
                encoding='utf-8',
                errors='replace',
                timeout=timeout
            )
            
            if use_spinner:
                spinner.busy = False
                t.join()
                sys.stdout.write(' Done\n')
                sys.stdout.flush()

            duration = time.time() - start_time
            self.logger.info(f"命令执行成功 (耗时 {duration:.2f}s)")
            self.logger.debug(f"输出:\n{result.stdout}")
            return True, result.stdout
        except subprocess.CalledProcessError as e:
            if use_spinner:
                spinner.busy = False
                t.join()
                sys.stdout.write(' Failed\n')
                sys.stdout.flush()
            self.logger.error(f"命令执行失败 (退出码 {e.returncode})")
            self.logger.error(f"错误输出:\n{e.stderr}")
            return False, e.stderr
        except subprocess.TimeoutExpired:
            if use_spinner:
                spinner.busy = False
                t.join()
                sys.stdout.write(' Timeout\n')
                sys.stdout.flush()
            self.logger.error(f"命令执行超时 ({timeout}s)")
            return False, "Timeout"
        except Exception as e:
            if use_spinner:
                spinner.busy = False
                t.join()
                sys.stdout.write(' Error\n')
                sys.stdout.flush()
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

    def upload(self):
        if not self.port:
            self.logger.error("未指定上传端口 (请使用 --port 或在 config.yaml 中配置)")
            sys.exit(11)
            
        # 检查端口是否存在 (简单检查)
        if self.os_type != "Windows" and not os.path.exists(self.port):
             self.logger.warning(f"端口 {self.port} 未在文件系统中检测到，尝试继续...")
        
        self.logger.info(f"开始上传到端口: {self.port}")
        
        # 判断是否使用预编译的固件文件
        if self.args and getattr(self.args, 'input_file', None):
            # 使用指定的固件文件上传
            input_file = self.args.input_file
            if not os.path.exists(input_file):
                self.logger.error(f"指定的固件文件不存在: {input_file}")
                sys.exit(14)
            self.logger.info(f"使用预编译固件: {input_file}")
            cmd = [self.arduino_cli, "upload", "-p", self.port, "--fqbn", self.fqbn, 
                   "--input-file", input_file, self.sketch]
        else:
            # 普通上传（指定编译时的输出目录，防止缓存找不到问题）
            build_path = None
            if self.args and getattr(self.args, 'build_path', None):
                build_path = self.args.build_path
            else:
                build_path = self.config.get('default', {}).get('build_path')
                
            cmd = [self.arduino_cli, "upload", "-p", self.port, "--fqbn", self.fqbn]
            
            if build_path:
                if not os.path.isabs(build_path):
                    base_dir = os.path.dirname(os.path.abspath(self.args.config if self.args else 'config.yaml'))
                    build_path = os.path.join(base_dir, build_path)
                cmd.extend(["--input-dir", build_path])
                
            cmd.append(self.sketch)
        
        success, _ = self.run_command(cmd, message="正在上传... ")
        if not success:
            self.logger.error("上传失败，终止流程")
            sys.exit(12)
        return True

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
        if not self.port and self.reset_method in ["dtr_rts", "1200bps"]:
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
        if not self.port:
            self.logger.error("未指定串口")
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
        
        # 如果没有指定任何操作，默认显示帮助
        if not (self.args.compile or self.args.upload or self.args.serial or self.args.regress_reset):
            self.logger.warning("未指定任何操作。请使用 -c, -u, -s 参数。")
            return

        # 1. 编译
        if self.args.compile:
            self.compile()

        # 2. 上传 (如果只指定上传，也会执行；如果指定了编译+上传，编译失败会终止)
        if self.args.upload:
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

def main():
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
    parser.set_defaults(auto_reset=None)
    
    args = parser.parse_args()
    
    # 确定配置文件绝对路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = args.config if os.path.isabs(args.config) else os.path.join(script_dir, args.config)
    
    # 初始化日志 (先加载配置以获取日志路径)
    # 这里为了简化，先读取一次配置或使用默认
    log_file = os.path.join(script_dir, "mus4/ArduinoCLI.log")
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
