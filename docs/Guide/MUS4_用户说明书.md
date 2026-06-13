# MUS4 用户说明书

本文面向调试和操作 MUS4 固件的人。读完后，应能连接设备、确认串口数据、进入 Web Console、执行安全维护命令，并理解哪些行为是当前固件已经实现的。

## 1. 安全前提

MUS4 会直接控制转向舵机和油门电调。上电、上传固件、发送控制命令或做标定前，先让车辆离地或断开驱动负载，并确认 Park 处于锁定状态。

不要把来自 USB Serial、RS232 Serial1、TCP Console 或 Web Console 的输入视为可信输入。所有会影响车辆状态的无线命令都应经过认证和 Park 条件检查。

## 2. 串口连接

USB Type-C 对应 Arduino `Serial`，波特率 115200。它用于本地命令、ACK/NACK、日志和 TUI 输出。默认日志目标启用 Web Console 时可能不是 USB Serial；需要本地观察日志时发送 `LOG_SERIAL`。

RS232 对应 `Serial1`，RX/TX 为 GPIO 16/17，波特率 115200。它接收 Pilot 控制帧，也输出车辆控制遥测：

```text
Txx:Sxx
```

DEV mode 会保持 OTA window 打开，但不会再阻止 `Serial1` 遥测。只有 OTA 实际上传过程中，`Serial1` 遥测会暂停。

## 3. 控制帧

Pilot 输入支持三种格式：

```text
Throttle:Steering
Throttle:Steering:Seq
Throttle:Steering*XX
```

`Throttle` 和 `Steering` 范围为 `-100..100`。成功返回 `ACK` 或 `ACK:Seq`，失败返回 `NACK` 或 `NACK:Seq`。

## 4. Web Console

设备 AP 可用时，连接设备 Wi-Fi 后打开：

```text
http://192.168.4.1/
```

Web Console 当前提供状态查看、命令发送、串口/系统日志、曲线数据、STA 配置、DEV mode、OTA 上传入口和 Tub JSON 记录。若设备连接到 STA 网络，页面会显示 STA IP；可在同一局域网打开该 IP 访问 Web Console。

## 5. OTA

HTTP OTA 使用 Web Console `/update` 端点。执行 OTA 前需要满足权限与 Park 条件；DEV mode 可放宽 Web 入口认证，但不会取消 Park 相关安全保护。OTA 实际传输时固件会进入保护状态并暂停 `Serial1` 遥测。

常用状态命令：

```text
OTA_STATUS
DISABLE_OTA
```

## 6. 常用命令

```text
PING
STATUS
AUTH:<password>
LOG_SERIAL
LOG_WEB
WIFI_STA_STATUS
WIFI_STA_SSID:<ssid>
WIFI_STA_PASSWORD:<password>
WIFI_STA_APPLY
WIFI_STA_CLEAR
```

诊断、基准、回归和转向标定命令属于维护命令，需要认证和 Park 锁定。

## 7. 代码入口

固件入口是 `MUS4_FW.ino`。项目级声明集中在 `MUS4.h`。实现按职责分成 IO、Control、Command、Wi-Fi 四个 `.cpp` 文件。Web Console 页面资产保留在 `WebConsoleAssets.h`，因为它是生成式 HTML/CSS/JS 资源，不适合作为人工阅读 API 的一部分。

## 8. 当前未覆盖的内容

本文只描述当前代码中已经存在的功能。未来功能和整理方向放在 `docs/Plan/ROADMAP.md`，不代表当前固件已经实现。
