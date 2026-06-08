# HTTP Update OTA 上传操作说明

本文说明 MUS4 固件通过 **HTTP `/update` 端点**进行 OTA 上传的操作流程。该方式区别于传统的 `ArduinoOTA`（端口 3232），直接通过 Web Console 的 HTTP 接口上传 `.bin` 文件，支持浏览器拖放和命令行 `curl.exe`。

## 与 ArduinoOTA 的区别

| 特性 | HTTP Update OTA（本文） | ArduinoOTA（旧方式） |
|------|------------------------|---------------------|
| 传输协议 | HTTP POST multipart | 自定义 TCP 协议 |
| 端口 | 80（Web Console 共用） | 3232 |
| 瓶颈 | 无 1KB/ACK 瓶颈，传输更快 | 受 1KB/ACK 握手限制 |
| 进度显示 | curl 原生进度条 / 浏览器进度条 | espota.py 模拟进度条 |
| 依赖 | 仅需 curl.exe（Windows 自带） | 需要 Python + espota.py |
| 认证 | 复用 Web Console 认证 + Park 锁定 | 独立的 OTA 密码 |

## 前置条件

1. 设备已烧录支持 HTTP `/update` 的固件（Web Console 已启用）。
2. 电脑与设备在同一网络（连接 `MUS4-DEBUG` AP 或同一 STA 局域网）。
3. 已生成主固件 `.bin` 文件，例如 `build_wsl\mus4.ino.bin`。
4. Windows 10/11 自带 `curl.exe`（PowerShell 中请使用 `curl.exe` 而非 `curl`）。

## 一、在 Web Console 中开启上传权限

HTTP `/update` 端点复用现有的安全策略，上传前必须满足以下任一条件：

- **开发模式已开启**（`Auto OTA` 开关为 ON），或
- **已认证**（发送 `AUTH:mus4-debug`）且 **Park 处于 LOCKED 状态**

### 步骤

1. 打开 Web Console：
   ```
   http://<设备IP>/
   ```

2. 点击 **"OTA Upload"** 按钮，或在浏览器直接访问：
   ```
   http://<设备IP>/update
   ```

3. 将 `.bin` 固件文件**拖放**到页面区域，或点击"选择文件"。

4. 等待上传完成，设备会自动重启。

> **提示**：如果未认证或 Park 未锁定，上传会被拒绝。在 Web Console 首页发送 `AUTH:mus4-debug` 和 `ENABLE_OTA` 即可。

## 二、命令行使用 curl.exe

### 基本命令

```powershell
curl.exe -X POST http://192.168.3.140/update `
  -H "Content-Type: multipart/form-data" `
  -F "firmware=@build_wsl\mus4.ino.bin" `
  --progress-bar
```

> **注意**：PowerShell 中必须使用 `curl.exe`，`curl` 会被解析为 `Invoke-WebRequest`。

### 使用 `.mus4_ota_target` 自动获取目标 IP

项目根目录的 `.mus4_ota_target` 文件可存放默认目标 IP：

```powershell
# 先读取目标 IP
$target = (Get-Content .mus4_ota_target)[0].Trim()
curl.exe -X POST "http://$target/update" -F "firmware=@build_wsl\mus4.ino.bin" --progress-bar
```

## 三、使用 WSL 构建脚本一键上传

### 编译 + HTTP OTA 上传

```powershell
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost 192.168.3.140
```

### 使用 `.mus4_ota_target` 免输入 IP

如果项目根目录已存在 `.mus4_ota_target` 文件：

```powershell
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta
```

脚本会自动读取 `.mus4_ota_target` 第一行作为目标主机。

### 仅上传已有固件（不重新编译）

```powershell
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost 192.168.3.140
```

### 常用参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-HttpOta` | 启用 HTTP OTA 上传（区别于 `-Ota`） | 无 |
| `-HttpOtaHost` | HTTP OTA 目标 IP 或主机名 | 读取 `.mus4_ota_target` |
| `-Compile` / `-c` | 编译固件 | 无 |
| `-Upload` / `-u` | 上传固件 | 无 |
| `-Clean` | 清理后重新编译 | 无 |

## 四、完整推荐流程

使用 WSL 构建并直接 HTTP OTA 上传：

```powershell
# 1. 编译 + HTTP OTA 上传（自动读取 .mus4_ota_target）
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta

# 2. 上传成功后设备自动重启，约 5-10 秒后恢复运行
# 3. 重新连接 Web Console，确认 STATUS 正常
```

## 五、故障排查

### 提示 "HTTP OTA 需要指定目标主机"

处理方法：
- 添加 `-HttpOtaHost <IP>` 参数，或
- 在项目根目录创建 `.mus4_ota_target` 文件，第一行写入设备 IP：
  ```
  192.168.3.140
  ```

### curl.exe 返回 exit code 22 或 HTTP 403/500

curl 的 `--fail` 会在 HTTP 状态码 >= 400 时返回 22。

检查顺序：

1. **认证**：在 Web Console 发送 `AUTH:mus4-debug`。
2. **Park 锁定**：确认 Park 状态为 LOCKED。如果未锁定，发送 `ENABLE_OTA`（会自动锁定）。
3. **开发模式**：如果已开启开发模式（`Auto OTA` 开关为 ON），可免认证免 Park 上传。
4. **网络连通性**：`Test-NetConnection 192.168.3.140 -Port 80` 是否成功。

### 上传后设备无响应

- 确认上传的是**主固件**（`mus4.ino.bin`），而非 `.bootloader.bin` 或 `.partitions.bin`。
- 检查 Web Console 的 `/update` 页面响应文本，如果显示 `ACK:UPDATE_OK`，说明写入成功，设备正在重启。
- 等待 10-15 秒后刷新 Web Console。

### curl.exe 找不到

Windows 10 1803+ 和 Windows 11 均自带 `curl.exe`。如果提示找不到：

```powershell
# 检查 curl.exe 是否存在
Get-Command curl.exe

# 如果不存在，可使用 PowerShell 的 Invoke-WebRequest（不推荐，multipart 支持较差）
```

## 六、安全注意事项

- HTTP `/update` 端点与 Web Console 共用认证体系，**未认证且非开发模式时无法上传**。
- 上传期间固件会自动强制 `Park Locked`，防止电机意外动作。
- 建议在受控调试网络中使用，不要在公共网络暴露 Web Console。
- 开发模式会持久化到 NVS，生产部署前请确认已关闭。
