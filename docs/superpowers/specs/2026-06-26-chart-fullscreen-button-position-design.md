# Web Console 曲线区域全屏按钮位置优化设计

## 背景

MUS4 Web Console 的曲线区域（`#chartPanel`）目前把「全屏 / 退出全屏」切换按钮放在 canvas 下方的 `.chartToolbar` 工具栏里。用户希望把它移动到曲线区域（canvas）的右下角，并随曲线画布缩放自动调整位置，始终保持在右下角。

> 用户所称的「放大 / 缩小按钮」即现有的全屏 / 退出全屏切换按钮。

## 设计目标

1. 全屏按钮始终位于曲线画布的右下角。
2. 画布尺寸变化（窗口缩放、进入 / 退出全屏、DPR 变化）时，按钮位置自动跟随。
3. 不引入额外依赖，不改动按钮功能与安全路径。

## 方案选择

考虑过三种方案：

- **A. 画布内右下角贴边**：按钮绝对定位在 canvas 内部右下角（`right: 8px; bottom: 8px`）。紧凑，但可能轻微遮挡曲线末端数据。
- **B. 画布内右下角留白**：在 canvas 右侧和底部为按钮留出边距，绘图时同步留出空白。不遮挡数据，但需调整绘图边距。
- **C. 面板容器右下角**：按钮放在 canvas 外的 `#chartPanel` 右下角。不遮挡，但视觉上不属于曲线区域。

**最终选择：方案 A**。用户明确要求按钮位于「曲线区域右下角」，贴边方案最直接满足该要求，且实现改动最小。

## 详细设计

### 视觉布局

```
+----------------------------------+
|                                  |
|            曲线画布               |
|                                  |
|                         [⛶]     |  <-- 全屏按钮，距右 8px，距底 8px
+----------------------------------+
[暂停][清空][录制][下载]  图例
```

### 样式

- 按钮容器：`position: absolute; right: 8px; bottom: 8px;`
- 定位基准：新增 `.chartCanvasWrap` 容器包裹 canvas 与按钮，并设置 `position: relative;`
- 按钮尺寸：保持现有 `.iconButton` 大小（约 28px 图标区），不随画布缩放
- 层级：`z-index: 2` 高于 canvas，确保不被曲线覆盖
- 样式类复用现有 `.iconButton`，保持视觉一致

### HTML 结构调整

在 `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h` 中：

1. 将 `chartFullscreenBtn` 从 `.chartToolbar` 中移除。
2. 新增 `.chartCanvasWrap` 容器，包裹 `<canvas id="chart">` 与 `chartFullscreenBtn`。
3. 将 `chartFullscreenBtn` 作为绝对定位元素放在 canvas 右下角。

### JavaScript 调整

- 保留 `chartFullscreenBtn` 引用与 `toggleChartFullscreen()` 函数，功能不变。
- `refreshDynamicLabels()` 仍需更新按钮图标与 tooltip（全屏 ↔ 分屏）。
- `initCanvasDpr()` 与 `fullscreenchange` 事件已触发重绘，按钮位置由 CSS 自动维护，无需额外计算。

### 响应式行为

- 窗口缩放：`.chartCanvasWrap` 尺寸随 `#chartPanel` 变化，按钮绝对定位自动保持在其右下角。
- 进入 / 退出全屏：`fullscreenchange` 事件触发后，canvas 重新计算尺寸，按钮仍保持在 `.chartCanvasWrap` 右下角。
- DPR 变化：`initCanvasDpr()` 重新设置 canvas 内部尺寸，不影响按钮 CSS 位置。

## 文件变更

- `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`
  - HTML 结构：调整 `#chartPanel` 内部元素顺序与按钮位置。
  - CSS：增加 / 调整定位相关样式。
  - JS：无功能变更，仅需确认 `refreshDynamicLabels` 仍能获取到按钮引用。
- `MUS4_FW/tests/test_firmware_feature_flags.py`
  - 更新全屏模式 CSS 断言，并新增 `.chartCanvasWrap` 结构与按钮定位断言。

## 验证

1. 打开 Web Console，确认全屏按钮显示在曲线画布右下角。
2. 调整浏览器窗口大小，按钮始终保持在 canvas 右下角。
3. 点击按钮进入全屏，按钮应切换到退出全屏图标，位置仍在全屏后画布的右下角。
4. 退出全屏后，按钮恢复原始图标与位置。
5. 编译验证：`python arduino-cli.py -c --sketch MUS4_FW.ino` 无错误。

## 安全说明

本改动仅涉及 UI 布局，不影响控制命令权限、Park Locked、OTA 安全路径或无线策略。

## 参考

- 当前实现：`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`
- 相关计划：`MUS4_FW/docs/Plan/WebConsole串口与动态曲线分阶段方案.md`
