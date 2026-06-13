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
