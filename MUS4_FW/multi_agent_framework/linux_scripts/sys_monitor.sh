#!/bin/bash
# 智能体生成的 Linux 系统监控与日志处理脚本 (符合 POSIX 标准)
# 功能：系统负载、内存、磁盘IO、网络流量、温度监控与日志轮转

LOG_DIR="/var/log/mus4"
LOG_FILE="$LOG_DIR/sys_monitor.log"
POLL_INTERVAL=5
MAX_LOG_SIZE=$((5 * 1024 * 1024)) # 5MB 日志大小限制

# 初始化环境
mkdir -p "$LOG_DIR"
touch "$LOG_FILE"

# 错误处理机制
trap 'log_msg "[ERROR] 脚本执行中断，发生错误"; exit 1' ERR
trap 'log_msg "[INFO] 监控服务已安全停止"; exit 0' SIGINT SIGTERM

log_msg() {
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    echo "[$timestamp] $1" | tee -a "$LOG_FILE"
}

# 1. 日志轮转机制 (Log Rotation)
rotate_logs_if_needed() {
    local file_size=$(stat -c%s "$LOG_FILE" 2>/dev/null || echo 0)
    if [ "$file_size" -gt "$MAX_LOG_SIZE" ]; then
        mv "$LOG_FILE" "${LOG_FILE}.$(date +%Y%m%d%H%M%S).bak"
        touch "$LOG_FILE"
        log_msg "[INFO] 日志文件已轮转"
        # 仅保留最近3个备份
        ls -t ${LOG_FILE}.*.bak | tail -n +4 | xargs rm -f 2>/dev/null
    fi
}

# 2. 系统核心指标监控 (CPU / 内存 / 温度)
check_system_health() {
    # 内存使用率
    local mem_usage=$(free | awk '/Mem/ {printf("%.2f"), $3/$2 * 100.0}')
    
    # CPU 负载 (1分钟)
    local cpu_load=$(top -bn1 | grep load | awk '{printf "%.2f", $(NF-2)}')
    
    # 系统温度 (适配多数 Linux 板卡，如 Lattepanda / 树莓派)
    local temp="N/A"
    if [ -f "/sys/class/thermal/thermal_zone0/temp" ]; then
        local raw_temp=$(cat /sys/class/thermal/thermal_zone0/temp)
        temp=$(awk "BEGIN {printf \"%.1f\", $raw_temp/1000}")
    fi
    
    log_msg "[HEALTH] 内存: ${mem_usage}% | CPU负载: ${cpu_load} | 温度: ${temp}°C"
    
    # 告警阈值
    if (( $(echo "$mem_usage > 90.0" | bc -l) )); then
        log_msg "[WARN] 内存使用率过高! (${mem_usage}%)"
    fi
}

# 3. 磁盘 I/O 监控
check_disk_usage() {
    local disk_usage=$(df -h / | awk 'NR==2 {print $5}' | sed 's/%//')
    if [ "$disk_usage" -gt 85 ]; then
        log_msg "[WARN] 根目录磁盘空间不足! 已使用: ${disk_usage}%"
    fi
}

# 4. 网络状态与流量监控
check_network() {
    # 检查默认网关连通性
    local gateway=$(ip route | awk '/default/ {print $3}' | head -n1)
    if [ -n "$gateway" ] && ping -c 1 -W 1 "$gateway" &> /dev/null; then
        local net_status="Online"
    else
        local net_status="Offline"
        log_msg "[ERROR] 默认网关 $gateway 无法连通"
    fi
    
    # 简单统计网卡 rx/tx 流量 (以 eth0/wlan0 为主，取活跃网卡)
    local active_iface=$(ip route get 1.1.1.1 2>/dev/null | awk '{print $5}' | head -n1)
    if [ -n "$active_iface" ]; then
        local rx_bytes=$(cat /sys/class/net/$active_iface/statistics/rx_bytes 2>/dev/null || echo 0)
        local tx_bytes=$(cat /sys/class/net/$active_iface/statistics/tx_bytes 2>/dev/null || echo 0)
        log_msg "[NET] 接口: $active_iface | 状态: $net_status | RX: $rx_bytes B | TX: $tx_bytes B"
    fi
}

# 主循环
main() {
    log_msg "=== MUS4 高级 Linux 系统监控服务启动 ==="
    
    while true; do
        rotate_logs_if_needed
        check_system_health
        check_disk_usage
        check_network
        
        # 记录心跳分隔符
        echo "---------------------------------------------------" >> "$LOG_FILE"
        sleep "$POLL_INTERVAL"
    done
}

# 确保以正确权限运行
if [ "$EUID" -ne 0 ]; then
  echo "请使用 sudo 运行此监控脚本，以获取完整的硬件访问权限"
  exit 1
fi

main
