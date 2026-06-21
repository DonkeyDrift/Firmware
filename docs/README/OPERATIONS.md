# MUS4 运行操作速查

完整说明见 `docs/Guide/MUS4_用户说明书.md`。

## 串口

- USB `Serial`: 115200 baud，用于本地命令、ACK/NACK、TUI/log 输出。
- RS232 `Serial1`: 115200 baud，用于 Pilot 输入和 `Txx:Sxx` 遥测输出。

`Serial1` 遥测在 DEV mode 的 OTA window 打开时继续输出；只有 OTA 实际传输中暂停。

## 常用命令

```text
PING
STATUS
AUTH:<password>
LOG_SERIAL
LOG_WEB
OTA_STATUS
DISABLE_OTA
WIFI_STA_STATUS
```

诊断、回归、转向标定类命令需要 Park 锁定。无线入口还需要满足认证策略。
# 操作手册
- 启动
  - 连接Type‑C与RS232，波特率115200
  - 首次渲染完成后进入增量更新模式
- 降级模式
  - 触发后终端显示“DEGRADED MODE ACTIVE”
  - 关闭波形、降低刷新频率，仍可接收控制帧
- 测试与验证
  - 在任一串口输入命令并回车：
    - `TEST` 输出用例统计与通过率
    - `BENCH` 输出UI循环基准结果
    - `STRESS` 输出异常压力统计
    - `REGRESS` 输出回归校验结果
- 数据帧
  - 基本：`Throttle:Steering\n`，范围−100..100
  - 可选：`payload*CS\n`，CS为两位十六进制校验
  - 回执：`ACK`/`NACK`
- Serial1 遥测回传帧
  - 格式：`T:S\n`，范围−100..100
  - `T` 代表 Throttle，`S` 代表 Steering
  - 仅在 MANUAL 模式下发送；ASSIST / AUTO 模式下关闭 TX 遥测
  - 刷新率约 60 Hz
  - 含义：回传最近一次从 Serial1 接收到的控制指令数值
- 维护
  - 传感器异常不阻塞主循环
  - 自适应刷新频率自动调节
