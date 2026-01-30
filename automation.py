#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import argparse
import subprocess
import logging
import time
import platform
import yaml
import shutil
import threading
import itertools

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
    return logging.getLogger("Automation")

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
        self.logger = logging.getLogger("Automation")
        self.config = self.load_config(config_path)
        self.args = args
        
        # 参数优先级：命令行 > 配置文件 > 默认值
        self.arduino_cli = args.cli or self.config.get('default', {}).get('arduino_cli', 'arduino-cli')
        self.fqbn = args.fqbn or self.config.get('default', {}).get('fqbn', 'esp32:esp32:esp32')
        self.port = args.port or self.config.get('default', {}).get('port', '')
        self.baud = args.baud or self.config.get('default', {}).get('baudrate', 115200)
        self.sketch = args.sketch or self.config.get('default', {}).get('sketch_path', '')
        
        # 转换 sketch 路径为绝对路径
        if not os.path.isabs(self.sketch):
            base_dir = os.path.dirname(os.path.abspath(config_path))
            self.sketch = os.path.join(base_dir, self.sketch)
            
        # 检测操作系统
        self.os_type = platform.system()
        self.validate_environment()

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
        
        # 检查 arduino-cli
        if not shutil.which(self.arduino_cli):
            self.logger.error(f"找不到命令: {self.arduino_cli}")
            sys.exit(2)
            
        # 检查 sketch 文件
        if not os.path.exists(self.sketch):
            self.logger.error(f"找不到 Sketch 文件: {self.sketch}")
            sys.exit(3)

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
        cmd = [self.arduino_cli, "compile", "--fqbn", self.fqbn, self.sketch]
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
        cmd = [self.arduino_cli, "upload", "-p", self.port, "--fqbn", self.fqbn, self.sketch]
        success, _ = self.run_command(cmd, message="正在上传... ")
        if not success:
            self.logger.error("上传失败，终止流程")
            sys.exit(12)
        return True

    def monitor(self):
        if not self.port:
            self.logger.error("未指定串口")
            sys.exit(13)
            
        self.logger.info(f"打开串口监控: {self.port} @ {self.baud}")
        self.logger.info("按 Ctrl+C 退出监控")
        
        cmd = [
            self.arduino_cli, 
            "monitor", 
            "-p", self.port, 
            "--config", f"baudrate={self.baud}"
        ]
        
        # 监控通常是交互式的，这里直接使用 subprocess.call 连接到 stdio
        # 或者如果是自动化测试模式，可以使用 timeout
        try:
            subprocess.run(cmd)
        except KeyboardInterrupt:
            self.logger.info("用户停止监控")
        except Exception as e:
            self.logger.error(f"监控异常: {e}")

    def run(self):
        total_start = time.time()
        
        # 如果没有指定任何操作，默认显示帮助
        if not (self.args.compile or self.args.upload or self.args.monitor):
            self.logger.warning("未指定任何操作。请使用 -c, -u, -m 参数。")
            return

        # 1. 编译
        if self.args.compile:
            self.compile()

        # 2. 上传 (如果只指定上传，也会执行；如果指定了编译+上传，编译失败会终止)
        if self.args.upload:
            self.upload()

        # 3. 监控
        if self.args.monitor:
            # 简单的延时确保串口已就绪
            if self.args.upload:
                time.sleep(1)
            self.monitor()
            
        total_duration = time.time() - total_start
        self.logger.info(f"所有任务完成，总耗时: {total_duration:.2f}s")

def main():
    parser = argparse.ArgumentParser(description="Arduino 项目自动化构建脚本")
    
    # 操作标志
    parser.add_argument('-c', '--compile', action='store_true', help='执行编译')
    parser.add_argument('-u', '--upload', action='store_true', help='执行上传')
    parser.add_argument('-m', '--monitor', action='store_true', help='打开串口监控')
    
    # 配置参数
    parser.add_argument('--port', '-p', help='串口设备路径 (e.g., /dev/ttyACM0, COM3)')
    parser.add_argument('--baud', '-b', type=int, help='串口波特率')
    parser.add_argument('--fqbn', help='板型定义 (FQBN)')
    parser.add_argument('--sketch', help='Arduino Sketch 文件路径')
    parser.add_argument('--cli', help='arduino-cli 可执行文件路径')
    parser.add_argument('--config', default='config.yaml', help='配置文件路径')
    
    args = parser.parse_args()
    
    # 确定配置文件绝对路径
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = args.config if os.path.isabs(args.config) else os.path.join(script_dir, args.config)
    
    # 初始化日志 (先加载配置以获取日志路径)
    # 这里为了简化，先读取一次配置或使用默认
    log_file = os.path.join(script_dir, "mus4/automation.log")
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
    
    automation = ArduinoAutomation(config_path, args)
    automation.run()

if __name__ == "__main__":
    main()
