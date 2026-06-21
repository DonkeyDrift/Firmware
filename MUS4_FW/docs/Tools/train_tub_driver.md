# MUS4 Tub JSON 训练工具

`tools/train_tub_driver.py` 用于读取 Web Console 导出的 tub JSON，生成数据质量报告，并训练一个 GRU 行为克隆 baseline。

## 能力边界

当前 tub 数据只包含遥测、RC 通道、控制输出和传感器字段，不包含摄像头图像或赛道几何信息。因此该工具产出的模型适合验证数据管线和控制行为建模，不等价于完整视觉自动驾驶模型。

若要训练真正的视觉自动驾驶模型，需要在 tub 中同步采集图像路径或图像帧，并把图像作为模型输入。

## 默认防泄漏策略

默认标签列：

- `thr`
- `str`

默认排除输入特征：

- `ch1`
- `ch2`
- `rct`
- `rcs`
- `thr`
- `str`
- `seq`
- `t`

其中 `ch1/ch2/rct/rcs` 基本就是遥控器操作输入，不能作为自动驾驶模型输入，否则模型会学到“复制遥控器”而不是驾驶策略。

如果采集了半自动或全自动数据，`pt/ps` 也可能造成泄漏，可追加排除：

```bash
python tools/train_tub_driver.py C:/Users/cross/Downloads/2136.json --add-exclude-columns pt,ps --dry-run
```

## 数据检查

```bash
python tools/train_tub_driver.py C:/Users/cross/Downloads/2136.json --dry-run
```

输出包括：

- 输入文件数
- 样本数
- 估算采样率
- 窗口大小
- 窗口数
- 标签列
- 特征列
- 已排除字段
- 数据质量警告

## 只生成报告

```bash
python tools/train_tub_driver.py C:/Users/cross/Downloads/2136.json --report-only --out-dir C:/Users/cross/Downloads/mus4_tub_report --overwrite
```

输出：

- `report.json`

## 训练 baseline

```bash
python tools/train_tub_driver.py C:/Users/cross/Downloads/2136.json --out-dir C:/Users/cross/Downloads/mus4_gru_baseline --epochs 50 --window-size 16 --overwrite
```

输出目录包含：

- `model.pt`：PyTorch 模型权重和配置。
- `standardization.json`：特征列、标签列、mean/std、窗口参数。
- `report.json`：数据质量报告和训练摘要。
- `predictions.csv`：验证集真实值与预测值。
- `prediction_curve.png`：`thr/str` 真实曲线与预测曲线。
- `loss_curve.png`：训练/验证 loss 曲线。

## 训练依赖

`--dry-run` 和 `--report-only` 不需要训练依赖。实际训练需要：

```bash
pip install torch numpy matplotlib
```

## 常用参数

- `--window-size`：历史窗口长度，默认 `16`。
- `--stride`：窗口步长，默认 `1`。
- `--target-offset`：标签相对窗口末尾的偏移，默认 `0`；设置为 `1` 可预测下一帧。
- `--epochs`：训练轮数，默认 `50`。
- `--batch-size`：批大小，默认 `64`。
- `--hidden-size`：GRU 隐层大小，默认 `64`。
- `--val-ratio`：验证集比例，默认 `0.2`，按时间顺序切分。
- `--no-plots`：跳过 PNG 曲线输出。

## 建议采集量

当前 30 秒级数据只能验证流程。若要上车低速验证，建议先采集至少 30～60 分钟，覆盖不同速度、电量、弯道和人为修正场景。
