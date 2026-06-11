# Web Console 版本号位置微调方案

## 背景

Web Console 顶部标题行已改为底边对齐，但实机视觉上版本号 `v1.5.21` 仍略显偏低。用户在视觉辅助对比中选择了上移 `1px` 的方案。

## 设计

仅调整 `.version` CSS：

```css
.version{color:#8fa1b5;font-size:12px;text-transform:uppercase;letter-spacing:.08em;display:inline-block;transform:translateY(-1px)}
```

保持以下内容不变：

- `.headerRow{align-items:flex-end}`。
- 标题 `MUS4 Web Console` 的字体和位置。
- `DEV MODE` 开关的位置和文案。
- 版本号数值 `v1.5.21`。
- MODE/PARK 卡片布局。

## 测试

更新 `tests/test_firmware_feature_flags.py` 的源码断言，确保 `.version` 样式包含 `display:inline-block` 与 `transform:translateY(-1px)`。

## 验收标准

- 版本号相对当前效果上移 `1px`。
- 页面其它顶部元素不被移动。
- 相关源码断言测试通过。
