# Judge 第二批持久化评分参数设计

## 背景

当前 `/judge` 已完成第一批持久化调参入口，已经具备以下能力：

- 设备侧通过 `NVS/Preferences` 持久化 `collisionThreshold`、`bigTurnThreshold`、`windowSize`
- 页面通过现有 HTTP 接口读取、保存、恢复默认值
- 保存成功后立即作用于后续样本
- 设备重启后配置仍然保留

第一批参数解决了“判定边界”和“窗口长度”的问题，但在实车调参时仍然存在一个明显不足：

- 用户可以调“什么时候触发”，但还不能调“触发后扣多重”以及“6 个维度各自有多敏感”

用户已经明确确认本轮方向：

- 继续沿用设备侧 `NVS/Preferences` 持久化
- 不新增第二套配置接口
- 第二批参数同时包含：
  - `collisionPenalty`
  - 6 个维度各自 1 个评分敏感度参数
- 不把评分逻辑迁回固件，固件只负责配置持久化和下发

因此，本轮目标是在不把 `/judge` 复杂化为“大而全调参台”的前提下，补齐第二批真正影响评分“手感”的参数。

## 目标

- 在现有 Judge 配置结构上追加第二批评分参数
- 持久化到设备侧 `NVS/Preferences`
- 继续复用当前 Judge 配置 HTTP 接口
- 在 `/judge` 页面增加“评分参数”分组
- 保存后立即作用于后续评分样本
- 保持每个维度最多只新增 1 个可调参数

## 非目标

- 不开放每个维度的多个内部参数
- 不增加浏览器本地覆盖配置
- 不增加多套预设和配置导入导出
- 不把评分逻辑迁回固件侧
- 不引入新的 Judge 配置页面
- 不在本轮开放 `pseudoSpeed` 生成参数

## 方案对比

### 方案 1：`collisionPenalty` + 6 个维度单参数灵敏度

- 新增 1 个碰撞扣分参数
- 新增 6 个维度各自 1 个权重或敏感度参数
- 每个维度只保留 1 个调节点

这是本轮推荐方案。

优点：

- 参数数量可控
- 与当前页面评分函数一一对应
- 用户易于理解和试错
- 能明显改变评分手感

缺点：

- 仍需要用户理解“系数变大意味着更敏感”这层语义
- 某些复杂评分细节暂时不能单独微调

### 方案 2：`collisionPenalty` + 6 维灵敏度 + 更多内部系数

- 除了基础灵敏度，还开放更多公式内部参数
- 例如不同维度的阈值、分段系数、额外滞回因子等

优点：

- 自由度更高
- 更适合极致调参

缺点：

- 参数数量快速膨胀
- 页面复杂度明显上升
- 默认值、边界和测试成本显著增加

### 方案 3：接近全量评分器参数化

- 将当前评分器内大部分常量都暴露成配置项

优点：

- 可调自由度最高

缺点：

- 明显超出本轮合理范围
- 非常容易让调参体验失控
- 与“每轮最小闭环”目标冲突

## 最终决策

本轮采用方案 1：

- 新增 `collisionPenalty`
- 新增 6 个维度各自 1 个评分敏感度参数
- 继续复用现有 Judge 配置结构、HTTP 接口和 `/judge` 调参入口

## 配置模型

### 基础原则

第二批参数继续遵循以下约束：

- 固件负责保存和下发
- 前端负责评分逻辑使用
- 每个维度最多增加 1 个新增参数
- 参数命名尽量直白，保持和页面评分函数一一对应

### 新增字段

建议在现有 Judge 配置上增加以下字段：

```cpp
float collisionPenalty;
float turnSmoothnessWeight;
float rangeMatchWeight;
float gyroStabilityWeight;
float bigTurnStabilityWeight;
float speedStabilityWeight;
float throttleStabilityWeight;
```

### 字段语义

#### `collisionPenalty`

作用：

- 每次碰撞触发时，从总分中扣减固定值

语义：

- 值越大，碰撞的总分惩罚越重

#### `turnSmoothnessWeight`

作用：

- 控制 `calcTurnSmoothness()` 中转弯变化率对得分衰减的强弱

语义：

- 值越大，对抖动越敏感

#### `rangeMatchWeight`

作用：

- 控制 `calcRangeMatch()` 中 `pseudoSpeed` 与 `|gyroZ|` 不匹配时的扣分强度

语义：

- 值越大，动作与代理速度不协调时掉分越快

#### `gyroStabilityWeight`

作用：

- 控制 `gyroHistory` 的波动对“陀螺稳定”维度的惩罚强度

语义：

- 值越大，对陀螺波动越敏感

#### `bigTurnStabilityWeight`

作用：

- 控制进入大弯区后，最近窗口波动对“大弯稳定”维度的惩罚强度

语义：

- 值越大，大弯阶段波动越容易掉分

#### `speedStabilityWeight`

作用：

- 控制 `pseudoSpeed` 波动对“速度稳定”维度的惩罚强度

语义：

- 值越大，代理速度不稳定时掉分越明显

#### `throttleStabilityWeight`

作用：

- 控制油门波动对“油门稳定”维度的惩罚强度

语义：

- 值越大，对抽油门越敏感

## 默认值与边界

### 默认值原则

默认值统一沿用当前页面中已经验证可工作的常量强度，确保：

- 默认状态下评分行为与当前版本尽量一致
- 旧用户升级后不会立刻感知到评分风格突变

### 边界原则

所有第二批参数都必须做设备侧范围校验。

约束如下：

- `collisionPenalty` 限定在小范围正值区间内
- 6 个维度系数限定在小范围正浮点区间内
- 不允许为负数
- 不允许为 0
- 不允许为明显过大的极端值

原因：

- 负值会反转评分逻辑
- 零值会让某些维度失去意义
- 极端大值会让单个维度过度敏感，导致评分不可用

### 范围策略

推荐采用“默认值附近的小范围可调”策略，而不是宽松开放：

- 保持调参结果可预测
- 降低误操作风险
- 避免页面成为实验性参数面板

## 生效策略

### 保存后立即生效

与第一批参数保持一致：

- 保存成功后立即更新运行时 Judge 配置
- 后续新样本立刻使用新参数
- 不要求重启设备

### 不回溯重算

本轮仍然不回溯重算当前已经累计的旧样本。

即：

- 当前一局已累计的历史样本不重算
- 保存后的新参数只影响之后的新样本

### 恢复默认值

恢复默认值时：

- 第一批基础阈值和第二批评分参数一起回到默认配置
- 同时写回设备侧 `Preferences`

## 接口设计

### 路由策略

继续复用现有接口：

- `GET /api/judge-config`
- `POST /api/judge-config`
- `POST /api/judge-config/reset`

不新增第二套接口路径。

### 读取配置

`GET /api/judge-config` 返回内容扩展为两组参数：

- 基础阈值
- 评分参数

同时继续返回：

- 默认值集合
- 参数范围集合

### 保存配置

`POST /api/judge-config` 在现有 3 个基础字段之上，新增接收：

- `collisionPenalty`
- `turnSmoothnessWeight`
- `rangeMatchWeight`
- `gyroStabilityWeight`
- `bigTurnStabilityWeight`
- `speedStabilityWeight`
- `throttleStabilityWeight`

行为要求：

- 字段必须完整
- 固件必须做最终校验
- 校验通过后统一写入运行时和 `NVS/Preferences`

### 恢复默认值

`POST /api/judge-config/reset` 的行为改为：

- 同时重置基础阈值与第二批评分参数
- 返回完整当前默认配置

## 页面设计

### 分组方式

`/judge` 调参区拆成两组：

#### 基础阈值

- `collisionThreshold`
- `bigTurnThreshold`
- `windowSize`

#### 评分参数

- `collisionPenalty`
- `turnSmoothnessWeight`
- `rangeMatchWeight`
- `gyroStabilityWeight`
- `bigTurnStabilityWeight`
- `speedStabilityWeight`
- `throttleStabilityWeight`

### 交互方式

页面行为继续保持和第一批一致：

1. 页面加载时读取完整配置
2. 表单展示当前设备值
3. 用户编辑后点击保存
4. 保存成功后页面状态提示为“已保存并立即生效”
5. 点击恢复默认值后两组参数一起回到默认值

### 文案策略

页面中应尽量减少“weight”或“公式系数”带来的理解门槛。

建议在 UI 上用更面向调参的中文说明，例如：

- `碰撞扣分`
- `转弯平滑敏感度`
- `区间匹配敏感度`
- `陀螺稳定敏感度`
- `大弯稳定敏感度`
- `速度稳定敏感度`
- `油门稳定敏感度`

## 前端评分逻辑映射

### 映射原则

第二批参数只替换当前评分函数里的固定常量，不改变评分函数的大体结构和命名语义。

### 对应关系

- `collisionPenalty` 对应碰撞触发时的总分扣减值
- `turnSmoothnessWeight` 对应 `calcTurnSmoothness()` 中变化率惩罚强度
- `rangeMatchWeight` 对应 `calcRangeMatch()` 中不匹配惩罚强度
- `gyroStabilityWeight` 对应 `calcGyroStability()` 中方差惩罚强度
- `bigTurnStabilityWeight` 对应 `calcBigTurnStability()` 中大弯窗口波动惩罚强度
- `speedStabilityWeight` 对应 `calcPseudoSpeedStability()` 中 `pseudoSpeed` 波动惩罚强度
- `throttleStabilityWeight` 对应 `calcThrottleStability()` 中油门波动惩罚强度

## 校验与错误处理

### 固件侧校验

固件必须校验：

- 字段是否齐全
- 是否为有效数值
- 是否落在允许范围内

任何不合法输入都必须拒绝写入。

### 页面错误反馈

页面保存失败时：

- 保留用户输入
- 给出明确错误提示
- 不自动回退成设备旧值

## 测试与验证

### 源码断言

更新 `tests/test_firmware_feature_flags.py`，确保覆盖：

- Judge 配置结构中出现第二批字段
- `WifiManager` 读写这些字段
- `WebConsoleServer` 接口返回这些字段
- `/judge` 页面新增第二组表单并在评分函数中消费这些配置项

### 回归验证

继续执行：

- `pytest tests/test_firmware_feature_flags.py`
- `.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino`

目标：

- 保证扩展后的 Judge 配置不破坏现有构建
- 保证 `/judge` 页面增强不破坏现有遥测链路

## 本轮不做项

- 每个维度开放多个内部参数
- 配置导入导出
- 多套参数预设
- 浏览器临时覆盖
- 独立调参页
- 固件侧评分器实现
- `pseudoSpeed` 参数持久化

## 成功标准

- `/judge` 页面能读取并展示第二批评分参数
- 用户可以保存 `collisionPenalty` 和 6 个维度敏感度参数
- 保存后后续评分样本立即体现变化
- 设备重启后配置仍然保留
- 恢复默认值后基础阈值和评分参数一并恢复
- 非法输入不会污染设备配置
