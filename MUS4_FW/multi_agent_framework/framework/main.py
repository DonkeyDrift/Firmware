import queue
import time
from agents import RequirementAgent, PlanningAgent, DevelopmentAgent, IntegrationAgent, TestingAgent
import logging

def main():
    print("==================================================")
    print("启动嵌入式多智能体协作开发框架 (Embedded-Multi-Agent)")
    print("==================================================")

    # 1. 初始化消息队列 (IPC)
    q_req_to_plan = queue.Queue()
    q_plan_to_dev = queue.Queue()
    q_dev_to_int = queue.Queue()
    q_int_to_test = queue.Queue()
    q_final_delivery = queue.Queue()

    # 2. 统一配置管理
    global_config = {
        "esp32_target": "esp32s3",
        "idf_version": "v5.1",
        "linux_target": "ubuntu_22.04",
        "posix_strict": True
    }

    # 3. 实例化智能体
    agent_req = RequirementAgent("Requirement_Agent", queue.Queue(), q_req_to_plan, global_config)
    agent_plan = PlanningAgent("Planning_Agent", q_req_to_plan, q_plan_to_dev, global_config)
    agent_dev = DevelopmentAgent("Development_Agent", q_plan_to_dev, q_dev_to_int, global_config)
    agent_int = IntegrationAgent("Integration_Agent", q_dev_to_int, q_int_to_test, global_config)
    agent_test = TestingAgent("Testing_Agent", q_int_to_test, q_final_delivery, global_config)

    agents = [agent_req, agent_plan, agent_dev, agent_int, agent_test]

    # 4. 启动所有智能体
    for agent in agents:
        agent.start()

    # 5. 提交初始需求任务
    initial_request = {
        "project": "MUS4_Dashboard",
        "description": "ESP32 AP mode, WebSocket server, Linux monitoring daemon, Web control panel"
    }
    agent_req.input_queue.put({"type": "ANALYZE_REQ", "payload": initial_request})

    # 6. 等待最终交付物 (阻塞监控)
    delivery = q_final_delivery.get()
    print(f"\n[框架监控中心] 项目开发流程全部完成! 交付物版本: {delivery['payload']}")

    # 优雅关闭
    for agent in agents:
        agent.input_queue.put({"type": "STOP"})
    
    time.sleep(1)
    print("系统退出。")

if __name__ == "__main__":
    main()
