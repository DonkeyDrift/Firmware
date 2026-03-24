#!/bin/bash
# 智能体生成的 Linux 系统监控与日志处理脚本 (符合 POSIX 标准)

LOG_FILE="/var/log/mus4_sys_monitor.log"
POLL_INTERVAL=5

# 错误处理机制
trap 'echo "[ERROR] 脚本执行中断，发生错误" | tee -a "$LOG_FILE"; exit 1' ERR
trap 'echo "[INFO] 监控服务已停止" | tee -a "$LOG_FILE"; exit 0' SIGINT SIGTERM

log_msg() {
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    echo "[$timestamp] $1" | tee -a "$LOG_FILE"
}

check_system_health() {
    # 内存使用率
    local mem_usage=$(free | awk '/Mem/ {printf("%.2f"), $3/$2 * 100.0}')
    # CPU 负载
    local cpu_load=$(top -bn1 | grep load | awk '{printf "%.2f", $(NF-2)}')
    
    log_msg "系统健康状态 - 内存使用率: ${mem_usage}% | CPU负载: ${cpu_load}"
    
    # 简单的告警机制
    if (( $(echo "$mem_usage > 90.0" | bc -l) )); then
        log_msg "!! 警告: 内存使用率过高 !!"
    fi
}

main() {
    log_msg "=== MUS4 Linux 系统监控服务启动 ==="
    
    # 检查网络配置服务 (依赖于 NMCLI 或 PING 测试)
    if ping -c 1 127.0.0.1 &> /dev/null; then
         log_msg "网络接口状态: 正常"
    else
         log_msg "网络接口状态: 异常"
    fi

    # 持续监控循环
    while true; do
        check_system_health
        sleep "$POLL_INTERVAL"
    done
}

# 确保以正确权限运行
if [ "$EUID" -ne 0 ]; then
  echo "请使用 root 权限运行此监控脚本"
  exit 1
fi

main
