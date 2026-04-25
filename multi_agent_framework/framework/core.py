import queue
import threading
import time
import json
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

class BaseAgent(threading.Thread):
    """基础智能体类，支持基于消息队列的IPC"""
    def __init__(self, name, input_queue, output_queue=None, config=None):
        super().__init__()
        self.name = name
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.config = config or {}
        self.logger = logging.getLogger(self.name)
        self.daemon = True

    def run(self):
        self.logger.info(f"{self.name} 启动...")
        while True:
            try:
                task = self.input_queue.get(timeout=1)
                if task.get("type") == "STOP":
                    break
                self.process_task(task)
                self.input_queue.task_done()
            except queue.Empty:
                continue

    def process_task(self, task):
        raise NotImplementedError("子类必须实现process_task方法")

    def send_message(self, target_queue, message):
        if target_queue:
            target_queue.put(message)
            self.logger.debug(f"发送消息到队列: {message.get('type')}")
