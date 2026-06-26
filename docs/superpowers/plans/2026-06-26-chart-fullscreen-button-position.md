# Web Console 曲线区域全屏按钮位置优化实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 Web Console 曲线区域的全屏/退出全屏按钮从底部工具栏移动到 `<canvas>` 右下角，并随画布缩放保持右下角位置。

**Architecture:** 新增 `.chartCanvasWrap` 容器包裹 `<canvas>` 与 `chartFullscreenBtn`，并设置 `position: relative`，把 `chartFullscreenBtn` 改为绝对定位（`right: 8px; bottom: 8px`），使其以 canvas 区域为基准自动跟随尺寸变化。HTML 中把按钮从 `.chartToolbar` 移入 `.chartCanvasWrap`，CSS 控制悬浮位置，JS 功能保持不变。

**Tech Stack:** Arduino C++ PROGMEM 内嵌 HTML/CSS/JS（`MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`）

---

## 文件变更

- **Modify:** `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`
  - HTML：把 `chartFullscreenBtn` 从 `.chartToolbar` 移入新增的 `.chartCanvasWrap` 容器（与 `<canvas>` 同层）。
  - CSS：给 `.chartCanvasWrap` 加 `position:relative`，给 `#chartFullscreenBtn` 加绝对定位样式。
  - JS：无需修改行为，仅确认 `refreshDynamicLabels()` 仍能拿到 `chartFullscreenBtn` 引用。

---

### Task 1: 调整 HTML 结构

**Files:**
- Modify: `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h:30-32`

**当前代码：**

```html
<section class="panel" id="chartPanel">
<canvas id="chart" width="760" height="260"></canvas>
<div class="chartFooter"><div class="chartToolbar"><button class="iconButton" onclick="toggleChart()" id="chartBtn" title="暂停">...</button><button class="iconButton" onclick="clearChart()" title="清空">...</button><button class="iconButton" onclick="toggleChartFullscreen()" id="chartFullscreenBtn" title="全屏">...</button><button class="iconButton" onclick="toggleTub()" id="tubRecordBtn" title="开始录制">...</button>...
```

- [ ] **Step 1: 把 `chartFullscreenBtn` 从 `.chartToolbar` 中移除**

删除该段中第三个 `<button class="iconButton" onclick="toggleChartFullscreen()" id="chartFullscreenBtn" ...>...</button>`。

预期变更后 `.chartToolbar` 内的按钮顺序为：暂停、清空、录制、下载。

- [ ] **Step 2: 用 `.chartCanvasWrap` 包裹 canvas 与全屏按钮**

把 `<canvas>` 行改为：

```html
<div class="chartCanvasWrap">
<canvas id="chart" width="760" height="260"></canvas>
<button class="iconButton" onclick="toggleChartFullscreen()" id="chartFullscreenBtn" title="全屏"><svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M0 0h5L0 5z"/><path d="M16 0h-5L16 5z"/><path d="M0 16h5L0 11z"/><path d="M16 16h-5L16 11z"/></svg></button>
</div>
```

注意保留原有 SVG 图标与 `id="chartFullscreenBtn"`，方便 JS 继续引用。

---

### Task 2: 增加 CSS 定位样式

**Files:**
- Modify: `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`（第一个 `<style>` 块，约第 12 行）

- [ ] **Step 1: 给 `.chartCanvasWrap` 增加相对定位基准**

在现有 CSS 中找到 `#status{...}` 或 `.panel{...}` 附近，追加：

```css
.chartCanvasWrap{position:relative}
```

- [ ] **Step 2: 给全屏按钮增加绝对定位样式**

继续追加：

```css
#chartFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}
```

由于按钮本身已有 `.iconButton` 类提供尺寸与背景样式，这里只需要位置与层级。

---

### Task 3: 验证 JS 引用与动态标签更新

**Files:**
- Modify: `MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h`（相关 JS 段落）

- [ ] **Step 1: 确认 `chartFullscreenBtn` 仍被 JS 获取**

`refreshDynamicLabels()` 中通过 `document.getElementById('chartFullscreenBtn')` 获取按钮（或顶层 `const` 初始化列表中已包含）。因为按钮 `id` 未变，所以无需修改。

- [ ] **Step 2: 确认 `refreshDynamicLabels()` 仍能更新按钮图标与 tooltip**

函数体中应包含：

```javascript
f.innerHTML=document.fullscreenElement===chartPanel?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN;
f.title=document.fullscreenElement===chartPanel?t('button.split'):t('button.fullscreen');
```

因为 `f` 是通过 `document.getElementById('chartFullscreenBtn')` 获取的，按钮位置改变不影响此逻辑，无需修改。

---

### Task 4: 编译验证

**Files:**
- Test: `MUS4_FW/arduino-cli.py`

- [ ] **Step 1: 进入子项目目录并运行编译**

Run:

```bash
cd MUS4_FW
python arduino-cli.py -c --sketch MUS4_FW.ino
```

Expected: 编译成功，无 HTML/CSS/JS 语法错误导致的 PROGMEM 字符串解析错误。

- [ ] **Step 2: 运行现有 Python 测试套件**

Run:

```bash
cd MUS4_FW
pytest tests/
```

Expected: 所有现有测试通过（本改动不修改后端逻辑）。

---

### Task 5: 手动浏览器验证（可选，需连接设备或本地调试）

- [ ] **Step 1: 烧录固件或本地启动 Web Console 页面**

- [ ] **Step 2: 确认按钮位置**

打开 Web Console，观察全屏按钮是否显示在曲线画布右下角。

- [ ] **Step 3: 确认响应式行为**

调整浏览器窗口大小，按钮应始终保持在 canvas 右下角。

- [ ] **Step 4: 确认全屏切换**

点击按钮进入全屏，按钮图标应变为退出全屏；再次点击退出全屏，图标恢复。

---

## 自我检查

- **Spec 覆盖：** 设计文档 `docs/superpowers/specs/2026-06-26-chart-fullscreen-button-position-design.md` 中所有要点（右下角定位、随画布缩放、功能不变、单一文件修改）均已对应到任务。
- **无占位符：** 所有步骤均给出具体代码、命令与预期结果，无 "TBD"/"TODO"。
- **类型一致性：** `chartFullscreenBtn` 的 `id` 在 HTML、JS 初始化、动态标签更新中保持一致。
