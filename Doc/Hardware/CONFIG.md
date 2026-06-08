# 配置说明

- 降级模式
  - 自动触发条件：传感器无效、UI渲染耗时>150ms、串口回写拥塞
  - 行为：关闭波形、降低UI刷新频率、保留核心控制与状态输出
  - 通知：串口打印“DEGRADED MODE ACTIVE”

- 刷新与缓存
  - UI自适应频率：100–500ms 动态调整
  - 传感器TTL：1000ms，仅TTL到期或数据变化时刷新
  - RC/输出行：值变化增量更新

- 串口通信
  - 帧格式：`<throttle>:<steering>` 或 `payload*CS`
  - 校验：CS为payload ASCII求和低8位，十六进制两位
  - 应答：有效帧回写`ACK`，无效帧回写`NACK`

- 测试命令
  - `TEST`：单元测试
  - `BENCH`：性能基准
  - `STRESS`：异常压力
  - `REGRESS`：回归校验

