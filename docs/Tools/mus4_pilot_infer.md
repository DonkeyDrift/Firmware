# MUS4 Pilot 模型推理控制器

`tools/mus4_pilot_infer.py` 用于在 Linux 主机上加载 `tools/train_tub_driver.py` 训练出的 GRU baseline，并通过串口向 ESP32 发送 MUS4 Pilot 控制命令。

## 安全边界

这是安全关键工具。默认模式是 `dry-run`，不会写串口。

实车控制前必须依次完成：

1. `dry-run`：只拉取 Web Console 数据并预测。
2. `zero-output`：打开串口，但只发送 `0:0:seq`。
3. 架空半自动 live：只允许转向，油门强制 0。
4. 架空全自动 live：极低油门限幅。
5. 地面低速短时 live。

任何阶段出现异常，都应停止，不要跳阶段。

## 模式

### dry-run

```bash
python tools/mus4_pilot_infer.py \
  --esp32-url http://192.168.3.39 \
  --model-dir /home/dkc/mus4/models/mus4_gru_baseline \
  --mode dry-run \
  --rate-hz 5 \
  --duration-sec 60
```

### zero-output

```bash
python tools/mus4_pilot_infer.py \
  --esp32-url http://192.168.3.39 \
  --model-dir /home/dkc/mus4/models/mus4_gru_baseline \
  --serial-port /dev/serial/by-id/<actual-device> \
  --baud 115200 \
  --mode zero-output \
  --rate-hz 10 \
  --duration-sec 60
```

### live

`live` 必须显式传入：

```bash
--i-understand-risk
```

半自动架空测试：

```bash
python tools/mus4_pilot_infer.py \
  --esp32-url http://192.168.3.39 \
  --model-dir /home/dkc/mus4/models/mus4_gru_baseline \
  --serial-port /dev/serial/by-id/<actual-device> \
  --baud 115200 \
  --mode live \
  --i-understand-risk \
  --max-throttle 0 \
  --max-steering 20 \
  --duration-sec 60
```

## 控制协议

脚本发送：

```text
throttle:steering:seq\n
```

期望 ESP32 返回：

```text
ACK:seq
```

## 安全门控

- `park == 1`：强制输出 `0:0`。
- `mode == 0`：强制输出 `0:0`。
- `mode == 1`：油门强制 `0`，只允许转向。
- `mode == 2`：允许油门和转向，但受限幅和速率限制。
- NaN/Inf、HTTP 失败、串口失败、ACK 超时都会进入 fail-safe。

## 远端部署目录

推荐：

```text
/home/dkc/mus4/
├── repo/
├── models/mus4_gru_baseline/
├── logs/pilot/
└── venv/
```

## 远端依赖

```bash
python3 -m venv ~/mus4/venv
~/mus4/venv/bin/pip install --upgrade pip
~/mus4/venv/bin/pip install torch requests pyserial numpy
```

## 串口权限

查看设备：

```bash
ls -l /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/* 2>/dev/null || true
```

如果设备属于 `dialout`：

```bash
sudo usermod -aG dialout dkc
```

重新登录后生效。

## 日志

可使用：

```bash
--log-file /home/dkc/mus4/logs/pilot/pilot.ndjson
```

日志为 NDJSON，每行记录最新遥测、预测、命令和 ACK 状态，不记录密码。
