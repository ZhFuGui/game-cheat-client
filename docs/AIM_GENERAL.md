# Aim 常规模块

2026-11-01补充：所有Aim子页的不同H2组纵向排列；同组内部才允许并排。常规使用共用`aim-section-stack`，模块的算法/配置归属保持。Aimbot新增独立总开关与CS2三功能互斥，见[Aim功能选择](AIM_ACTIVATION.md)。

常规收纳可复用的计算模块、输入变换及其参数。一个模块可以由部分功能消费；无需Aimbot、Triggerbot、Rage全部使用才放入常规。模块的实际消费者由现有原生实现决定，界面归类不新增或合并控制行为。

2026-11-12补充：CS2新增运行条件模块，`cs2CheckForeground`默认true，旧配置缺键补true，false按游戏保存。它只控制Triggerbot/Rage原有的本机前台比较；关闭不伪造远端前台状态，不放宽协议、样本年龄、会话、设备、目标或真实射线条件，其他功能不新增前台门禁，ESP叠加层仍按其本机窗口策略。重新配置清除旧待发提议与分析。

2026-11-19补充：后座方案只保留原生求值。鼠标移动拆分几何轨迹与速度变化控制，新增四参数贝塞尔曲线，实际算法及旧配置迁移见[MOUSE_MOVEMENT_ALGORITHMS.md](MOUSE_MOVEMENT_ALGORITHMS.md)。

## 当前归属

| 游戏/模块 | 原有设置 | 实际作用范围 |
| --- | --- | --- |
| CS2 包含对象 | cs2RageEnemies、cs2RageTeammates | Aimbot、Triggerbot、Rage共用，默认只包含敌人，两项关闭时无可用目标 |
| CS2 运行条件 | cs2CheckForeground | Triggerbot、Rage共用本机前台检查，默认开启 |
| CS2 后座方案 | aimVersion=v1 | Aimbot、Rage共用原生 P/U 求值。补偿开关在 Aim → Aimbot → 后坐补偿 |
| CS2 鼠标移动算法 | mouseTrajectory、两个控制点、mouseMovementAlgorithm、speed | Aimbot、Rage共用直线/贝塞尔几何及独立速度控制 |
| CS2 鼠标计数换算 | cs2InputMapping、已有cs2Calibration | Aimbot、Rage共用换算记录 |
| CS2 时序与预测 | cs2Prediction、cs2AimMaxAgeMs、cs2ResponseMs | Aimbot、Rage共用。Triggerbot 的命中预测复用 cs2ResponseMs，采样年龄上限为 cs2TriggerMaxAgeMs |
| PUBG 后座方案/鼠标移动 | aimVersion=v1、mouseTrajectory、两个控制点、mouseMovementAlgorithm、speed | 原生发射参考、直线/贝塞尔、既有平滑时间控制 |
| PUBG 弹道计算 | zeroing、gravity、当前原生归零信息 | 现有PUBG计算模块；原生参考继续禁用手动归零 |
| PUBG 采样时序 | skew、livingLocalSkew | 现有Living/Local配对与身体速度时差资格 |

Aimbot保留自身的部位/枪械分类部位、初选范围、目标优先级及触发侧键。Rage保留自身目标评分、掩体/其他人物判断和点击节奏。Triggerbot保留独立枪械、反应、掩体、年龄及点击条件。Aimbot 以同一个侧键进入目标跟随或手动方向的后坐补偿。两种情况使用 Aim 常规的换算、时序与速度。

PC2.8.50 将 Hitbox 内缩比例统一到常规的运行条件。Aimbot 使用内缩后的取样范围，Triggerbot 和 Rage 用它判断射线是否进入首次命中的 Hitbox 内侧。`cs2HitboxInsetPercent` 范围 0–90%，新配置默认 10%。旧配置优先迁移已启用模式保存的比例。该模式没有旧值时依次采用 Aimbot、Triggerbot、Rage 中首个已保存的值。已保存的共享比例优先，旧键只用于读取迁移。

2.8.42把包含对象移到常规第一组，沿用旧Rage存储键和勾选值。`targetFilterSettings`读入同一个`TargetFilter`，Aimbot和Rage通过`AimSettings`消费，Triggerbot用于首次Hitbox命中、反应扫描和提交复核。改变选项时先撤销输出，清理锁定、分析和待发请求。禁选人物的首次命中仍可挡住后方目标。

CS2依据：`games/cs2/rage/OutputProgram::configure`调用同一个`aimSettings`并组合原控制程序。共享值由各功能按其计算所需读取。PUBG目前无Rage，不能将常规分组表述为已新增其能力。

## 界面组织

- `pages/AimGeneral.tsx`只按当前游戏的`GameUiModule.aimGeneral`装配模块，不导入具体游戏表单。
- `AimGeneralModule`以稳定`id`和`component`登记真实模块，CS2为targets/runtime/solution/movement/mapping/timing，PUBG为solution/movement/ballistics/timing。
- `features/aim/common/`保存后坐字段、轨迹与速度选择、速度字段及设备能力等可复用展示；游戏适配组件提供可用算法与实际设置字段。
- `games/<id>/AimGeneralSettings.tsx`负责各游戏的时序、输入或弹道模块；`AimSettings`/`FollowSettings`只保留手动Aimbot参数。
- 现有`FeatureSection`负责无卡片、可折叠展示；通用页签框架只负责页面生命周期、固定首位与排序，不识别算法或消费者。

新增共享模块时，在对应游戏登记一项即可；已有模块通过组件组合复用，不从Aimbot页面借用表单或建立第二份状态。所属功能、适用范围与说明放在对应模块的ⓘ中。

## 存储和验证

早期模块迁移只改展示位置与源码归属；2.8.39新增的cs2CheckForeground按上方规则补默认并独立保存。`cs2AimMaxAgeMs`等历史键保留兼容。所有控件继续使用`useSettingBinding`，按当前游戏上下文保存到原有配置。切页仅挂载界面，不开启功能、写入默认值或改变运行资格；用户主动修改参数仍由原生执行原有撤销/清理规则。

验证覆盖两游戏常规的真实模块和手动页字段分离、原值恢复/显式保存、后坐基线及轨迹/速度独立保存、设备能力更新、固定页签与路由，以及隔离原生运行时中的模块挂载无写入和现有运动选择。默认直线数值由原黄金夹具核验；Reader与固件保持。
