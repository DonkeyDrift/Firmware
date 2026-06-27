# 漂移裁判系统 pseudoSpeed 合并设计

## 背景

当前 `Ref/` 下的漂移裁判系统原型由三部分组成：

- [`drift_judge.ino`](file:///c:/Dev/DDC/Firmware/MUS4_FW/Ref/drift_judge.ino) 负责启动独立 AP、读取 MPU6050，并通过 SSE 推送 JSON。
- [`index.html`](file:///c:/Dev/DDC/Firmware/MUS4_FW/Ref/index.html) 提供移动端评分页面。
- [`app.js`](file:///c:/Dev/DDC/Firmware/MUS4_FW/Ref/app.js) 实现 6 维评分、碰撞提示、权重调节和历史记录。

现有 `MUS4_FW` 主固件已经具备更完整的遥测基础设施：

- [`sampleWifiWebData()`](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino#L235-L270) 按固定周期采样 Web 遥测点。
- [`WebDataPoint`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_core/src/WifiConsoleTypes.h#L77-L102) 已包含 `gyroX/Y/Z`、`accelX/Y/Z`、油门、模式、漂移辅助状态等字段。
- [`/api/data`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp#L682-L709) 可输出历史点和最新状态。
- [`WebSocket schema v2`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebTelemetry.cpp#L147-L239) 可输出高频二进制遥测。

当前缺口在于：原型依赖 `rpm -> speed` 的简单映射，但主固件暂时无法直接提供真实速度。用户确认不追求真实测速，而是接受一个用于评分的“速度代理量”。

## 目标

- 为主固件新增正式遥测字段 `pseudoSpeed`，用于漂移裁判评分。
- `pseudoSpeed` 基于 `|gyro_z| + throttle` 合成，输出范围固定为 `0~100`。
- 保持 6 个评分维度名称尽量不变，仅将内部依赖 `speed` 的逻辑切换为 `pseudoSpeed`。
- 复用主固件现有 `/api/data` 和 WebSocket 遥测，不引入 `SSE + SPIFFS + 独立 AP` 新链路。
- 第一阶段将漂移裁判系统落为主固件内独立页面 `/judge`，与现有 `Drifter Console` 共用数据面。

## 非目标

- 不估算真实车速、轮速或里程。
- 不将 `pseudoSpeed` 伪装为 km/h、m/s 等物理单位。
- 不在第一阶段将评分逻辑下沉到固件执行。
- 不保留 `Ref/drift_judge.ino` 中的独立 Wi-Fi、SSE、SPIFFS 服务架构作为正式方案。
- 不调整现有控制、安全、OTA、认证与权限策略。

## 核心决策

### 1. 用评分代理量替代真实速度

`pseudoSpeed` 的语义是“当前漂移动态强度”，不是车体线速度。它用于解决“评分模型需要速度量纲，但当前系统没有真实速度输入”的问题。

这样定义有三个好处：

- 概念上诚实，不制造伪物理量。
- 调参目标明确，围绕“评分好不好用”而不是“测速准不准”。
- 可直接复用已有 IMU 与油门数据，避免额外硬件依赖。

### 2. 由固件统一输出 `pseudoSpeed`

`pseudoSpeed` 由固件统一计算并进入遥测结构，而不是由 Judge 页面单独在前端拼装。

原因：

- Judge 页面、Web Console、日志导出、后续数据录制都能复用同一字段。
- 避免前后端各算一套，导致调参与回放结果不一致。
- 便于后续把经验参数固化为统一接口契约。

### 3. 评分逻辑保留在前端

第一阶段只把基础代理量 `pseudoSpeed` 下沉到固件。6 维评分、碰撞检测、历史记录、权重调节仍保留在前端。

原因：

- 评分规则后续大概率频繁调参，前端迭代成本最低。
- 与 UI 联调更直接，适合快速试错。
- 固件职责保持在“提供稳定数据”，避免业务评分逻辑过早耦合到固件。

## 方案

### 方案概览

采用“固件标准化代理量 + Judge 页面评分”的双层结构：

1. 固件从 `gyro_z` 和 `throttle` 计算 `pseudoSpeed`。
2. `pseudoSpeed` 写入 `WebDataPoint`。
3. `/api/data` 和 WebSocket 一并输出该字段。
4. Judge 页面读取主固件遥测，将原先 `speed` 相关逻辑切换到 `pseudoSpeed`。

### `pseudoSpeed` 输入与输出

输入：

- `gyro_z`：来自 MPU6050 的偏航角速度，沿用主固件现有 SI 单位数据。
- `throttle`：来自 `car_output.throttle` 的标准控制量，范围 `-100~100`。

输出：

- `pseudoSpeed`：范围 `0~100` 的无量纲整数或浮点值，表示当前漂移动态强度。

推荐在 `WebDataPoint` 中以 `float` 存储，在前端展示时按需要 round。

### 推荐公式

先做归一化：

```text
gyroMag = abs(gyro_z)
gyroScore = clamp(gyroMag / gyroRef, 0, 1)
throttleScore = clamp(abs(throttle) / 100, 0, 1)
```

再做加权合成：

```text
pseudoRaw = gyroWeight * gyroScore + throttleWeight * throttleScore
```

再做稳定化：

```text
if pseudoRaw > filtered:
    filtered = filtered + riseAlpha * (pseudoRaw - filtered)
else:
    filtered = filtered + fallAlpha * (pseudoRaw - filtered)
```

最后映射：

```text
pseudoSpeed = clamp(filtered * 100, 0, 100)
```

### 建议初始参数

- `gyroWeight = 0.6`
- `throttleWeight = 0.4`
- `gyroRef = 依据实车日志标定的中等漂移代表值`
- `riseAlpha` 偏小，控制上升速度
- `fallAlpha` 更小，控制缓慢回落

参数原则：

- 偏稳定，不追求单帧响应。
- 只要是短时噪声、台架振动或轻微晃动，不应显著抬高 `pseudoSpeed`。
- 持续漂移或持续给油时，`pseudoSpeed` 应稳定进入中高区间。

### 死区与异常抑制

为避免静止振动或传感器毛刺造成误判，需要引入以下边界处理：

- `gyro_z` 设基础死区，小幅抖动按 0 处理。
- `throttle` 先限幅再参与计算。
- `mpu6050Data.valid == false` 时，`pseudoSpeed` 回落为 0。
- 单帧尖峰只影响 `pseudoRaw`，不应立即把 `pseudoSpeed` 冲高。

## 数据结构与接口改动

### 固件侧

建议新增一个轻量函数，例如：

- `compute_pseudo_speed()`

放置建议：

- 第一阶段可放在 [`MUS4_FW.ino`](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino) 的 Web 遥测采样附近，便于最小改动验证。
- 若后续稳定，可再下沉为 `libraries/mus4_web/` 或 `libraries/mus4_control/` 的独立小模块。

### `WebDataPoint`

在 [`WifiConsoleTypes.h`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_core/src/WifiConsoleTypes.h#L77-L102) 的 `WebDataPoint` 中新增字段：

```cpp
float pseudoSpeed;
```

### 采样链路

在 [`sampleWifiWebData()`](file:///c:/Dev/DDC/Firmware/MUS4_FW/MUS4_FW.ino#L235-L270) 中补充：

- 读取 `mpu6050Data.gyroZ`
- 读取 `car_output.throttle`
- 调用 `compute_pseudo_speed()`
- 将结果写入 `point.pseudoSpeed`

### HTTP 接口

在 [`WebConsoleServer.cpp`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleServer.cpp#L602-L709) 的 JSON 输出中增加：

- `latest` 中增加 `pseudoSpeed`
- `points` 历史点是否包含 `pseudoSpeed` 取决于前端需要

建议：

- `latest` 必须包含
- `points` 历史点第一阶段可以不包含，以控制载荷
- 若 Judge 页面需要历史曲线回放，再补充到 `points`

### WebSocket

在 [`WebTelemetry.cpp`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebTelemetry.cpp#L172-L239) 的 schema v2 中扩展 `latest` 区字段，加入 `pseudoSpeed`。

原则：

- 不破坏现有魔数与版本管理方式。
- 若字段追加导致 schema 变化，应升级版本号或保证前端解码明确感知新增字段。
- 第一阶段优先保证 Judge 页面可通过 HTTP `/api/data` 工作；WebSocket 适配可作为同步改动或紧随其后。

## Judge 页面接入

### 页面形态

第一阶段将漂移裁判系统落为独立页面 `/judge`，而不是直接并入现有 `Drifter Console` 标签页。

原因：

- 降低对现有 Web Console 结构的扰动。
- 便于单独迭代评分 UI 与交互。
- 后续若效果稳定，再决定是否整合为 `Drifter Console` tab。

### 资产组织

正式页面应并入 [`WebConsoleAssets.h`](file:///c:/Dev/DDC/Firmware/MUS4_FW/libraries/mus4_web/src/WebConsoleAssets.h) 的内嵌资产体系。

迁移原则：

- `Ref/index.html` 作为结构参考。
- `Ref/app.js` 作为评分逻辑参考。
- 不继续使用 SPIFFS 文件读取静态资源。

### 数据源

Judge 页面不再连接 `Ref` 的 `/sse`，而是改用：

- 首选：主固件 `/api/data`
- 可选增强：主固件 WebSocket 二进制遥测

### 前端职责

Judge 页面负责：

- 连接状态显示
- 实时得分显示
- 6 维评分计算
- 碰撞提示
- 权重调节
- 历史记录

Judge 页面不负责：

- 计算真实速度
- 维护第二套固件通信协议
- 重建独立 AP 或独立服务器

## 评分维度映射

在保留现有 6 个维度名称的前提下，内部语义调整如下：

### 1. 转弯平滑

- 继续主要依赖 `gyro_z`
- 关注偏航角速度变化率与连续性
- 目标是衡量漂移是否顺滑、是否突兀

### 2. 区间匹配

- 名称不变
- 内部由“真实速度与角速度匹配”改为“`pseudoSpeed` 与 `gyro_z` 匹配”
- 目标是衡量动态强度与姿态输出是否协调

### 3. 陀螺稳定

- 名称和主语义不变
- 继续看 `gyro_z` 波动与标准差
- 允许高动态区采用更宽容的惩罚曲线

### 4. 大弯稳定

- 名称不变
- 进入大弯状态时，不只看瞬时峰值，还要看 `gyro_z` 持续性与 `pseudoSpeed`
- 目标是减少尖峰误判

### 5. 速度稳定

- 名称保留
- 内部含义改为“`pseudoSpeed` 的稳定性”
- 关注整段漂移动态强度是否持续、是否忽高忽低

### 6. 油门稳定

- 名称和主语义不变
- 继续基于 `throttle` 的均值与波动
- 与 `速度稳定` 区分开，避免重复评分

## 异常处理

### 传感器异常

- `mpu6050Data.valid == false` 时，固件将 `pseudoSpeed` 置为 0。
- 前端在缺少关键数据时暂停相关维度累计，避免脏数据带来虚假评分。

### 数据断流

- Judge 页面在 HTTP / WebSocket 中断时，停止累计评分。
- 连接恢复后继续读取新数据，不将断流前后自动拼成同一次完整评分段。

### 静止振动

- 通过 `gyro_z` 死区与双速率平滑避免误判。
- 台架轻微震动不应产生高 `pseudoSpeed`。

### 参数失配

- 第一阶段允许在 Judge 前端快速调整评分阈值与权重。
- `pseudoSpeed` 本身的基础参数由固件统一维护。

## 测试策略

### 源码断言

修改 `tests/test_firmware_feature_flags.py`，新增或调整断言：

- `WebDataPoint` 中存在 `pseudoSpeed`
- `sampleWifiWebData()` 写入 `pseudoSpeed`
- `/api/data` 的 `latest` 输出 `pseudoSpeed`
- 如扩展 WebSocket schema，则断言编码与解码结构同步存在
- Judge 页面资产存在并引用主固件数据接口，而不是 `/sse`

### 台架验证

- 静止状态：`pseudoSpeed` 接近 0
- 小幅晃动：`pseudoSpeed` 不明显升高
- 持续转动开发板：`pseudoSpeed` 逐步升高
- 转动叠加给油：`pseudoSpeed` 高于单独给油或单独转动

### 实车验证

- 直线低速：`pseudoSpeed` 不应虚高
- 持续漂移：`pseudoSpeed` 稳定处于中高区间
- 短时抖动或轻碰撞：不应长时间高位维持

### 评分验证

- 同一段数据回放得分应一致
- `区间匹配` 与 `速度稳定` 的调参应围绕 `pseudoSpeed`
- `速度稳定` 与 `油门稳定` 不应高度重复

## 实施顺序

### 阶段 1：补固件代理速度字段

- 在 `WebDataPoint` 中加入 `pseudoSpeed`
- 在 `sampleWifiWebData()` 中计算并写入
- 在 `/api/data latest` 中输出

### 阶段 2：迁移 Judge 页面到主固件数据接口

- 以 `Ref/index.html` 和 `Ref/app.js` 为基础迁移
- 删除对 `/sse`、SPIFFS、独立 AP 的依赖
- 改为读取 `/api/data`

### 阶段 3：接入主固件页面路由

- 为 Judge 页面提供独立路由 `/judge`
- 保持与现有 `/` Web Console 并行

### 阶段 4：适配 WebSocket

- 如需要更高刷新率，再扩展 Judge 页面对 WebSocket schema 的支持

### 阶段 5：实车调参与回放校准

- 标定 `gyroRef`
- 调整 `gyroWeight / throttleWeight`
- 调整上升/下降滤波系数
- 调整 `区间匹配` 与 `速度稳定` 阈值

## 风险与权衡

- `pseudoSpeed` 不是真实速度，因此“区间匹配”与“速度稳定”的物理解释会弱化，但评分一致性会提高。
- 若参数未经过实车日志标定，第一版可能偏保守或偏敏感。
- 若 WebSocket schema 扩展处理不当，可能影响现有前端解码兼容性，因此应优先保证 `/api/data` 路径可用。
- 若过早把评分下沉到固件，会增加后续调参成本，因此本方案刻意保持前端评分、固件供数的边界。

## 结论

本方案选择以 `pseudoSpeed` 作为漂移裁判系统的正式代理速度字段，将原型系统中对 `speed` 的依赖替换为“基于 `|gyro_z| + throttle` 的动态强度量”。该字段由主固件统一计算并通过现有 Web 遥测接口输出，Judge 页面在前端继续负责评分算法与交互，最终以独立 `/judge` 页面形式接入 `MUS4_FW`。

这是当前约束下最小风险、最高复用、最易迭代的合并路径。
