# Living / Local 补偿时间差

PC2.0.4，2026-10-23。

Aimbot 页的“Living / Local 补偿时间差上限”控制 Local 相机位置与 Living 中己方完整身体速度的采样差。单位为毫秒，允许 0–5000 的整数，默认 100。0 表示采样区间端点必须相同，不表示关闭检查。输入框失焦或按 Enter 后保存，修改后松开侧键再按，使用新的配置代次。

配置键为 `values.livingLocalSkew`，随工作区保存在 `%LOCALAPPDATA%\Mousebox\workspaces\<工作区ID>\settings.json`。不同工作区独立，重开恢复已保存值。

## 判断方法

```text
差值毫秒 = max(
    abs(Local相机采样开始 - Living己方速度采样开始),
    abs(Local相机采样结束 - Living己方速度采样结束)
) / 1,000,000

差值毫秒 <= livingLocalSkew 时通过这一项检查。
```

这里使用同一 Reader 源时钟中的实际纳秒采样区间，包含真实依赖宽度；不比较两帧的网络接收时刻，不给任何一路人为增加延迟，也不缩窄上游来源区间。

界面原有“允许采样时间差”（键 `skew`）用于 Living / Local 整帧配对，与本项独立。新参数只传入相机位置补偿；同一 Living 目标的骨点与速度采样差仍按原 100ms 规则核验。既有数据年龄、时钟误差、探测期限、身份、原生 controller / basis 和字段当前性检查继续执行。

## 实现边界

- 参数范围和默认值位于 PUBG 的 `model/timing_settings.hpp`。
- PUBG 设置适配器负责验证、应用；`MotionConfiguration` 将当前值送入控制线程和只读诊断使用的 `LiveController`。
- `prepareBallisticAim` 只将此值应用于相机预测的 `PredictionPolicy.maximumVelocitySkewMs`；敌方目标预测使用原策略。
- 算法说明和时间诊断同时列出当前跨流上限与同一 Living 目标上限，不能继续显示固定的跨流100ms。
- Reader 契约、SDK、固件及串口能力不因此改变。

## 旧配置兼容

既有工作区文件没有新字段。PUBG 通过 `GameConfiguration.permitsAbsentValue` 明确声明仅 `livingLocalSkew` 可在旧文档中缺失，读取时补上兼容默认值100；只读加载不重写原文件。显式存在但类型或范围不合法的值仍拒绝载入，其他必需字段缺失仍报错。

软件连接成功等操作正常保存配置时，会把新增默认值一并写入工作区文件；原有参数保持其最新值。

该边界在通用存储层通过游戏配置接口执行，通用工作区存储不硬编码 PUBG 字段名，也不把完整文档无条件合并默认值来掩盖损坏。
