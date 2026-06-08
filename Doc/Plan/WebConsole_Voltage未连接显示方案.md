# Web Console Voltage 未连接显示方案

## 背景

`Voltage` 卡片当前只要收到大于 0V 的电压值，就按一位小数显示电压并计算剩余电量百分比。实机未接电池或电压采样悬空时，可能出现低于 5V 的无效读数；继续显示为电池电压会造成误解。

## 目标

- 当 Web Console 收到的 `vol` 小于 5V 时，`Voltage` 主值显示为 `未连接`。
- 小于 5V 时不再计算剩余电量百分比。
- 5V 及以上保持现有显示规则：一位小数电压、`REMAIN` 百分比、按百分比设置卡片状态色。
- 保持卡片布局、数据协议和后端采样逻辑不变。

## 非目标

- 不修改 INA219 或其它电压采样逻辑。
- 不修改 `/api/data`、WebSocket 二进制协议或状态输出字段。
- 不调整电量百分比公式。
- 不改变 Network、Drift、Mode、Park 等其它卡片。

## 设计

在前端 `updateState(p)` 中调整 Voltage 卡片渲染条件：

```javascript
const v=Number(p.vol);
if(!isNaN(v)&&v>=5){
  // 保持原有电压与百分比显示
}else{
  voltageValue.textContent='未连接';
  voltageSub.textContent='battery';
  voltageCard.className='stateCard driftOff';
}
```

选择在浏览器侧处理，是因为该需求只影响 Web Console 的展示语义，不需要改变固件数据采集或传输协议。阈值使用用户指定的 5V；等于 5V 仍视为有效连接，符合“小于 5伏”的边界描述。

## 测试策略

遵守 TDD，先更新 `tests/test_firmware_feature_flags.py` 的源码断言：

- 断言 Voltage 有效显示条件使用 `v>=5`。
- 断言小于阈值/无效值分支显示 `未连接`。
- 继续保留一位小数电压断言。
- 断言不再使用 `v>0` 作为有效电压显示条件。

验证命令：

```powershell
pytest tests/test_firmware_feature_flags.py -q
.\arduino-cli-wsl.ps1 -Compile
```

编译通过后，按项目偏好通过 HTTP OTA 上传。

## 风险与缓解

- **风险：5V 附近抖动导致显示切换。** 当前需求是展示阈值规则，先做最小实现；若后续实机观测到频繁闪烁，再增加滞回或滤波。
- **风险：低压但仍连接的异常电池被显示为未连接。** 车辆主电池正常范围远高于 5V，低于 5V 更符合未连接或采样异常语义。
- **风险：前端文案占用空间。** `未连接` 短于 IP/电压主值，不影响现有卡片布局。

## 验收标准

- `vol < 5` 时，Voltage 主值显示 `未连接`。
- `vol >= 5` 时，Voltage 主值继续显示一位小数电压。
- 小于 5V 时不显示百分比。
- 相关 Python 测试通过。
- 固件 WSL 编译通过。
