# Web Console Tub JSON 连续记录方案

## 背景

当前分支需要把 ESP32 采集到的通道数据生成 JSON tub 包，供后续模型离线分析。用户已确认 tub 文件由浏览器下载到电脑，不写入 ESP32 flash；样本范围需要持续记录，而不是单帧快照。

## 目标

- 在 Web Console 中提供 Tub 连续记录入口。
- 记录现有 Web 遥测数据点中的全部通道字段，包含 `ch1` 到 `ch6` 与时间戳 `t`。
- 停止记录后由浏览器生成并下载 `.json` 文件。
- 保持固件控制路径只读，不影响 Park、输出限幅、WebSocket 二进制协议和 `/api/data` 契约。

## 非目标

- 不在 ESP32 文件系统中持久化 tub 文件。
- 不新增无线控制命令。
- 不做后台无人值守长期采集；记录期间需要 Web Console 页面保持打开。
- 不改变现有曲线、日志、OTA 或控制命令行为。

## 数据格式

```json
{
  "schema": "mus4.web_data_point.tub.v1",
  "source": "mus4-web-console",
  "started_ms": 1000,
  "stopped_ms": 3500,
  "count": 2,
  "samples": [
    {
      "seq": 7,
      "t": 1234,
      "thr": 10,
      "str": -20,
      "mode": 1,
      "park": 0,
      "rct": 1510,
      "rcs": 1490,
      "ch1": 1490,
      "ch2": 1510,
      "ch3": 1000,
      "ch4": 1500,
      "ch5": 2000,
      "ch6": 1600,
      "pt": 8,
      "ps": -18,
      "cur": 123.4,
      "vol": 7.6,
      "gz": -0.12,
      "de": 1,
      "da": 1,
      "dc": -12.5,
      "gzf": 0.34
    }
  ]
}
```

字段说明：

- `schema`：tub 包格式版本。
- `source`：数据来源，固定为 `mus4-web-console`。
- `started_ms` / `stopped_ms`：浏览器记录开始与结束时看到的固件时间戳，使用样本中的 `t`。
- `count`：样本数量。
- `samples`：连续遥测样本，单个样本复用现有 Web Console 数据点短字段。

## 实现方案

1. 在 Python 镜像中新增 `format_tub_package(...)`，确保 tub 外层格式可以单测保护。
2. 在 Web Console 前端增加 `Start Tub`、`Stop Tub`、`Download Tub JSON` 控件。
3. 在现有遥测点进入页面数据路径后调用 `captureTubPoint(point)`。
4. 记录期间按 `seq` 去重后追加样本；停止后通过 `Blob` 和临时 `<a>` 下载 JSON 文件。
5. 设置 `TUB_MAX_SAMPLES = 20000`，达到上限后自动停止，避免浏览器内存无限增长。

## 安全边界

- 不修改 `rc_data`、`pilot_data`、`car_output`、`pwm_filtered`、`pwm_value`。
- 不修改 `park_change()`、OTA Park guard、`ledcWrite` 或 PWM 限幅路径。
- 不新增 ESP32 文件系统依赖，避免 flash 写入和分区风险。
- 不改 WebSocket 二进制协议字段顺序，不改 `/api/data` 返回结构。

## 验证

```bash
pytest tests/test_wireless_console_policy.py -v
pytest tests/test_firmware_feature_flags.py -v
pytest tests/ -v
```

固件编译：

```powershell
.\arduino-cli-wsl.ps1 -Compile
```

HTTP OTA 目标：

```powershell
.\arduino-cli-wsl.ps1 -Upload -HttpOta -HttpOtaHost 192.168.3.39
```

## 手工验收

- 点击 `Start Tub` 后样本计数随遥测更新增长。
- 点击 `Stop Tub` 后样本计数停止增长。
- 点击 `Download Tub JSON` 后下载文件。
- 文件可被 JSON parser 解析。
- `count` 与 `samples.length` 一致。
- 每个样本包含 `t` 和 `ch1` 到 `ch6`。
- 记录和下载期间车辆输出与 Park 状态无额外变化。
