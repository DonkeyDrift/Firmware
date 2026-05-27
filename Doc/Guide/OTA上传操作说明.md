# OTA 上传操作说明

本文说明 MUS4 固件通过 Wi-Fi OTA 上传的操作流程。

## 前置条件

1. 设备已烧录支持 OTA 的固件。
2. 电脑已连接到 `MUS4-DEBUG` Wi-Fi，或与设备 STA IP 在同一网络。
3. 已生成主固件 `.bin` 文件，例如 `build_wsl\mus4.ino.bin`。
4. 已安装 Python、Arduino CLI、ESP32 core，并能在本机找到 `espota.py`。

如果 `espota.py` 无法自动发现，可用 `--espota-tool` 指定路径，或设置环境变量 `ESPOTA_PY`。

## 一、打开 OTA 窗口

OTA 上传前必须先通过 TCP Console 或 Web Console 打开 OTA 窗口。

连接设备 AP：

```text
SSID: MUS4-DEBUG
密码: mus4-debug
```

打开 Web Console：

```text
http://192.168.4.1/
```

依次发送：

```text
AUTH:mus4-debug
ENABLE_OTA
```

预期返回：

```text
AUTH_OK
OTA_READY ip=192.168.4.1 port=3232 ttl_ms=120000
```

OTA 窗口默认持续 120 秒，超时后需要重新执行 `ENABLE_OTA`。

## 二、执行 OTA 上传

推荐命令：

```powershell
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin
```

如果使用 STA 网络中的设备 IP，把 `--ota-host` 改为 `STATUS` 中显示的 `sta_ip`：

```powershell
python arduino-cli.py --ota --ota-host <设备STA_IP> --ota-password mus4-debug -i build_wsl\mus4.ino.bin
```

如需指定 `espota.py`：

```powershell
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin --espota-tool "C:\Users\<用户名>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<版本>\tools\espota.py"
```

上传时终端会显示 OTA 上传进度，例如：

```text
正在 OTA 上传... [==========          ] 50.0%
```

上传成功后设备会自动重启。

## 三、常用参数

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--ota` | 执行 OTA 上传 | 无 |
| `--ota-host` | OTA 目标 IP 或主机名 | 必填 |
| `--ota-port` | OTA 端口 | `3232` |
| `--ota-password` | OTA 密码 | 空字符串 |
| `-i`, `--input-file` | 要上传的主固件 `.bin` 文件 | 必填 |
| `--espota-tool` | 手动指定 `espota.py` 路径 | 自动发现 |
| `--no-progress` | 关闭终端进度条 | 默认显示 |

## 四、完整推荐流程

使用 WSL 构建固件：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

连接 `MUS4-DEBUG` 后，在 Web Console 中打开 OTA 窗口：

```text
AUTH:mus4-debug
ENABLE_OTA
```

在 120 秒内上传：

```powershell
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin
```

上传后重新连接 Web Console 或 TCP Console，发送：

```text
STATUS
```

确认设备已恢复运行。

## 五、故障排查

### 提示未指定任何操作

当前版本已支持只传 `--ota` 直接上传。若仍看到类似提示：

```text
未指定任何操作。请使用 -c, -u, -s 参数。
```

说明正在运行旧版本脚本。请确认当前分支包含 OTA CLI 修复。

### 提示需要指定固件文件

错误示例：

```text
OTA 上传需要通过 --input-file 指定固件 .bin 文件
```

处理方法：使用 `-i` 或 `--input-file` 指定主固件：

```powershell
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin
```

不要上传 `.bootloader.bin`、`.partitions.bin` 或 `.merged.bin`。脚本会尽量自动改用同目录主 `.bin`，但推荐直接指定 `mus4.ino.bin`。

### 提示需要指定 OTA 主机

错误示例：

```text
OTA 上传需要指定 --ota-host
```

处理方法：AP 模式通常使用：

```powershell
--ota-host 192.168.4.1
```

STA 模式使用 `STATUS` 中显示的 `sta_ip`。

### 找不到 espota.py

错误示例：

```text
找不到 espota.py，请使用 --espota-tool 指定路径或设置 ESPOTA_PY
```

处理方法：

```powershell
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin --espota-tool "<espota.py完整路径>"
```

或设置环境变量：

```powershell
$env:ESPOTA_PY = "<espota.py完整路径>"
python arduino-cli.py --ota --ota-host 192.168.4.1 --ota-password mus4-debug -i build_wsl\mus4.ino.bin
```

### OTA 连接失败或超时

检查顺序：

1. 电脑是否仍连接 `MUS4-DEBUG`。
2. `Test-NetConnection 192.168.4.1 -Port 3232` 是否成功。
3. OTA 窗口是否仍有效：Web/TCP Console 发送 `OTA_STATUS`。
4. 如果 `window=0`，重新发送：

```text
AUTH:mus4-debug
ENABLE_OTA
```

5. 确认设备处于 Park 锁定状态。`ENABLE_OTA` 要求已认证且 `park=1`。

### ENABLE_OTA 返回 NACK

- `NACK:UNAUTHORIZED`：尚未认证，先发送 `AUTH:mus4-debug`。
- `NACK:PARK_REQUIRED`：设备不在 Park 锁定状态，先切回 Park 锁定。

## 六、安全注意事项

- OTA 密码当前为调试密码 `mus4-debug`，只适合受控调试网络。
- OTA 窗口只在认证且 Park 锁定时打开。
- 上传前确认固件来自当前可信构建产物。
- 不要把真实 Wi-Fi 密码写入仓库；STA 凭据应放在已忽略的 `WirelessSecrets.h` 中。
