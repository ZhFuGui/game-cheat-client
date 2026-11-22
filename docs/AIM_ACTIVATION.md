# Aim 功能选择和布局

2026-11-01，PC2.8.37。

Aimbot页提供按游戏独立保存的`aimEnabled`。CS2的`aimEnabled`、`cs2TriggerEnabled`、`cs2RageEnabled`至多一个为true，也允许全部关闭。打开一个先停止旧输出，再关闭已运行控制器，最后配置新功能；关闭一个不会自动恢复之前的功能。原有参数、算法及按住侧键的资格保持。

原生`GameConfiguration::updatedValues`承接单次用户编辑产生的完整配置变更，CS2实现负责互斥。宿主只按游戏端口应用和保存，失败时沿用既有回滚流程；没有前端连续发送多条开关命令的中间并发窗口。旧配置normalize按Rage、Triggerbot、默认Aimbot顺序恢复既有owner，新开关默认true只是保留历史手动跟随能力；显式false继续保存，既有Rage和Triggerbot不会因新默认值而多启用Aimbot。

CS2模块切换输出所有者时先撤销设备会话资格，禁用旧Aimbot/Triggerbot/Rage/RCS控制器，再以最终值配置和绑定新owner，完成后恢复原会话资格。手动Aimbot的独立启用状态属于其OutputProgram实例，关闭清锁定及待反馈位移，提交复核拒绝旧提议；Rage组合的同类运动程序不受这个手动开关影响。切换不更改Rage共用的移动、后坐、速度等数值。独立RCS的优先级保持，Rage开启仍关闭独立RCS。

PUBG只增加实际可用的Aimbot启用开关。Backend在原有AutomaticAim入口及提交复核使用该状态，改变时递增配置代次并清旧锁定。Reader、ESP、桌面测试和明确调用的设备校准沿用各自规则。没有新增PUBG Triggerbot/Rage能力或Reader字段。

所有Aim子页的H2分组使用纵向`aim-section-stack`，组内继续使用自身并排/缩进规则，折叠仅改变显示。常规模块仍按游戏注册表装配，不因排版改变配置归属。标签说明是`PageTab.accessory`，页面框架按稳定tab ID装配；每个说明放在所属标签内部右侧，与实际tab按钮是同一视觉容器下的独立按钮，未选中也可查看。拖动变换作用于整个视觉容器，保留既有导航、排序、固定标签和键盘规则。
