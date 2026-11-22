# Reader Local 契约接入（PC0.10.0）

2026-10-16。本节取代下方历史196947A2契约作为当前源码入口；下面保留历史原验证范围，不将它们计为新Local现场验收。

冻结descriptor SHA256=`0B3C4F191B9A08EE89D0B25A03748A0E0A30794BB46CF69E6BDC5F9719A83D44`；来源`R:\pubg\analysis\reader\local-state-20261016\reader-consumer-contract-0b3c4f19.zip`，ZIP SHA256=`C90E53AD9AAF9CA9A65E9703090FBE6E63367CD345299C1DD59DD3A7993722E7`。完整冻结包57项内容加manifest共58文件，43份消费源码/头文件、15 CPP和10 proto逐SHA同步；新增local_validation.cpp参与VS工程，退役equipment.hpp/item.hpp/view.proto不再参与。Envelope2、时钟契约1保持。

- 类别6改为Local。FrameContext.groups仅World1/Map2/Coordinates3/Pacing5；Local在context.local提供control1、camera2、current_weapon3、spectators4。规范化内存组仍为LocalControl4、ViewSource6/ViewPose7/ViewTarget8、LocalWeapon9、LocalSpectators10；这些View*是相机子结构名称，不是旧网络类别。
- Living只含四个公共上下文，退役equipment属性组4和CharacterState.spectated_count7；health19已退役保持，骨架20、运动21与完整世界速度[4]保持。
- LocalControl.character_ref11提供全局注册表完整身份。消费者以reader/epoch/world/level/约定/时钟和id/actor/index/serial关联当前Living人物，导出self/team/controller等派生元数据，不伪造Living原始上下文或改写两流采样时间；不增加同帧屏障。Local断流/死亡/不匹配清派生控制信息，Living人物数据独立保留。
- CurrentWeaponValue的character1/processor2/active_slot3/raw_weapon_value4/weapon5/declared_slots6；active_slot是有符号原生槽位，0合法，负数、越界、原引用0不借其他槽位。LocalWeaponValue的地址1、class_name2、instance3、muzzle4；[5]证明当前武器，[5,4]独立证明枪口。仅Local当前本地身体可供枪，不借观战目标或旧装备。枪名失败不猜测，枪口失败不会撤销已核验枪名，也不阻断相机起点解算。
- LocalSpectatorsValue的character1/count2，只接受本地身体当前身份及非负int32人数，0与缺失分开。缺失/CLEAR不保留旧值；在状态API提供spectators，不增加多余界面控件。
- Local相机位置继续使用独立字段时间、最终旋转/FOV及原点；相机可在无身体时提供ESP视点，不能授予鼠标输出。相机失败时已核验武器和计数仍可独立消费。未声明新的FPP/TPP判断或真实枪口发射行为。

正式六套ReliableEnvelope在`tests/fixtures/reader-local-20261016`，各基线独立，死亡update只接当前武器baseline；另有`reader-local-named-20261016`命名武器/失败枪口夹具（独立manifest，不替换原六套）。夹具故意未设置坐标约定，不是运动就绪证据；纯关联测试补的已核验约定明确是合成数据。正式Reader组成层声明UNREAL_LEFT_HANDED_Z_UP，但没有声明verified_meters_per_unit，不补造0.01；已有控制100单位/米约定与ESP单位回退策略保持来源身份。

当前本地C++、官方Python交叉编码及TCP回环结果和实际配对部署状态见项目记忆与`out/local-contract-20261016`，此文不把离线通过写成游戏效果验收。

---

## 历史：Reader完整世界速度与枪口契约交接

2026-10-16。正式descriptor SHA256=`196947A2B345C89D6505E4FE37C3F33E4535E65642422721C0F5B407CF1028E3`，libprotoc35.1，Envelope2/clock1。41份消费源码/头文件（14个CPP）和10份proto全部核验、同步，上游源码保持原样。

来源ZIP：`R:\pubg\analysis\reader\motion-muzzle-20261016\reader-consumer-contract-196947a2.zip`，SHA256=`58980913E833868F20F3B731AD66FDF0E7BC9D57450391208C8A9C8DCB6ED47A`。UPSTREAM_MANIFEST保留原清单，SOURCE_MANIFEST固定41文件。`tools/check-reader-contract.ps1`校验来源和proto，不将工作树冻结冒称Git发行版。

## 消费规则

- CharacterMovementValue.world=4，组21允许掩码15；世界轴cm/s完整身体线速度，source1/2/3/4/5分别是运动、物理、骑乘替代、原生条件基座加法、明确零。贡献已计入，不能重复加。保留actor/component/carrier生命周期；不是任意骨点的动画或角速度。
- world使用本批独立观察`[4]`；world{}+失败证明是缺失，不是0。实时不再使用“组件速度+父对象假定0”冒充完整速度；旧二进制只保留原生运动诊断资格。
- WeaponValue.muzzle=6；world_transform.translation仍须加自己的source.origin一次。先以active_slot匹配slot.slot_index，再用数组ordinal i定位`[1,1,0x80000000|i,3,6]`，两种索引不能混用。仅当前本地武器生命周期、本批独立证明有效；失效/CLEAR撤销，缺挂点不回退Mesh。几何rotation不是子弹发射方向。
- View位置独立取ViewPose field_sources[1]的真实采样，不要求Living/View同时读取，以可靠映射分别对齐。
- 新版还要求spectated_count属于当前本地人物并有当前源样本；旧测试夹具补真实归属证明，不放宽上游校验。

## 正式夹具

Reader另交接6份完整ReliableEnvelope：基线、SET、合法零、子项失败、清字段、稀疏slot2/ordinal0新基线。ZIP SHA256=`BE4C1AE74632C1AAFE38C58C4D3B0879D0565D9EAD720C0F13DE823E766BBF90`。28项清单与原件在`tests/fixtures/reader-world-muzzle`，含官方Python解码及期望状态。

这些最小夹具不含有效LocalControl/View/骨架，不能伪造本地人物或输出资格；本项目另用完整合成状态验证当前武器投影。02—05均相对01固定基线，03未改枪口时应恢复111，不能残留02的211。

C++新增82项world/muzzle/稀疏槽位/独立证明/CLEAR检查；官方Python逐SHA核验28文件、完整重建6个状态并比较正式期望，另核验3份本项目导出记录。旧运动/观战/协议/时钟和五种本地TCP回环仍保留。以上是离线或本机回环，不是现场频率或游戏效果。

## 部署边界

人类明确授权“协调Radar后一并升级”后完成配对切换。新Reader SHA256=`530D50A23B9BAD8DC775C400E2D15F0F67580CDD792477838B41ACA2EF2BE9FA`，PID31900、创建UTC2026-10-15T23:16:43.0996360Z，Reader 局域网端口为 13102；游戏21816完整创建身份、stdin入口和既有白名单保持。新Radar196947同时安装、在Reader就绪后启动PID31080。精确旧Reader AB6E50F9/746609及旧Radar发布/配置备份保持；不动Hyper-V/DLL/EFI，不操作游戏或鼠标。

本项目Release只读客户端跨机12秒验证Living/View各3基线ACK、357更新、45心跳回显及11次时钟probe，零断线/失效/严格解析错误。当前地图TslLobby_Persistent_Main、is_lobby=true、人物0，View相机有效；因此只能确认链路与大厅流，不能说已观测world/muzzle游戏样本或已验收命中。现场需用户进入训练场/对局持枪后继续测试。部署和本机只读证据在out/ballistic-runtime-20261016；最终频率/服务收据以项目记忆为准。

最终恢复部署前Living/Vehicles/GroundLoot/View=30/1/1/30，全部configurationRevision9/applied，四类ready/0拒绝。恢复后本机另一次12秒只读新会话各3ACK/358更新/45心跳/11probe通过，仍为大厅空人物。两轮证据分别保留live-reader和live-reader-final，不覆盖第一轮记录。
