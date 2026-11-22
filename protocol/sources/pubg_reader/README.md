# Reader 订阅协议

正式入口：[synchronization.proto](synchronization/v3/synchronization.proto)。默认业务模型见 [consumer.proto](synchronization/v3/consumer.proto)，完整调试模型见 [debug.proto](synchronization/v3/debug.proto)。`protocol_version=3`、`clock_contract_version=1`；Reader 与消费者须配对升级，不提供 v2 回退。

```text
protocol/
├─ foundation/common.proto
└─ synchronization/v3/
   ├─ synchronization.proto    握手、ACK、调频、失效、心跳、时钟探测
   ├─ consumer.proto           强类型业务基线、固定基线增量与真实时间
   ├─ debug.proto              完整内部证据、覆盖与任务诊断
   └─ model/
      ├─ consumer_values.proto Consumer专用业务模型
      ├─ shared.proto          以下文件为Debug丰富模型
      ├─ living.proto
      ├─ vehicle.proto
      ├─ ground_loot.proto
      ├─ local.proto           相机、本地控制、当前武器与观战数
      ├─ container.proto       当前未实现
      └─ projectile.proto      当前未实现
```

## Consumer 与 Debug

Hello.profile 默认 `CONSUMER=1`，显式 `DEBUG=2` 才接收完整内部证据。profile/features 在握手冻结，修改须重连。当前仅支持 `features=0`（该类别全部已实现业务能力）；非零值明确返回 `Unsupported`，不会暗中启用不完整的功能级规划。

Consumer 使用 Envelope 字段 19/20 和 `business_kinds` 能力合同，不携带 Native Identity。Debug 使用字段 7/11 和丰富属性合同；不得以假地址、假 serial 适配旧客户端。两者共享已验证的 ACK、固定基线、栅栏、原子提交和失效内核，强类型状态分别验证。

同一消费者/订阅/类别在不同 profile 下可共存。同 profile 重连仅替换自身；最新状态、序号、失败下界和 Hz 分 profile 隔离，共享全局世界 epoch 与实体生命周期 ID。两种 profile 的类别请求分别取最大值，基础发现使用总需求最高值；Debug2Hz 不会让 Consumer60Hz 的纯调试链按60Hz读取。

Consumer 从已验证 Snapshot 直接构造强类型业务 DTO，不先建立完整 Debug Value/证明树再删除。不传原生地址、serial、来源槽位、分类规则、解释证据、参考骨架或局部矩阵；内部必要核验继续执行。保留不透明实体/owner 引用，以及 controller、weapon、camera、骨骼布局/pose、原生输入上下文 token。

Consumer 每次更新完整刷新当前业务组的终态和真实来源时间，业务值只传相对已 ACK 固定基线的变化。缺失清除旧值；0、false 和空字符串可以存在。来源时间不得从父组或前一增量借用；历史移除原因不属于基线当前状态。

`ConsumerGroupRecord.sources` 路径从该组的具体业务 message 起算，不包含 ConsumerGroupValue 的 oneof 包装字段号。普通数字为字段号，最高位为1的数字是零起点 repeated 索引。Context.sources 使用 ConsumerContextValues 字段路径。动态嵌套值使用自身原始 source，完整表每次刷新。

骨骼只传20个语义主干点的世界位置和语义父点，不携 native bone index。role=1..20 顺序为骨盆、脊柱1/2/3、颈、头、左锁骨/上臂/前臂/手、右锁骨/上臂/前臂/手、左大腿/小腿/脚、右大腿/小腿/脚。跨流引用尚未解析时保留来源实体，不能伪造关联或清空其他类别。

以下时间、网络、速度和 Local 业务语义两种 profile 通用；提及原生地址、掩码、关闭证据或丰富路径的细节仅适用于 Debug。Consumer 对应强类型字段以 consumer_values.proto 为准。

## 接入与局域网

Reader 原生提供 TCP 服务，无需 reader-bridge。默认 `127.0.0.1:13102`。局域网运行示例：

```powershell
.\pubg-hyperv-memory-reader.exe --listen-address 0.0.0.0 --allow-peer 192.168.1.0/24
# 也可绑定具体本机 IPv4，只允许另一台机器：
.\pubg-hyperv-memory-reader.exe --listen-address 192.168.1.10 --allow-peer 192.168.1.20
```

`--listen-port` 可改变端口；`--allow-peer` 可重复，接受明确的 IPv4 或规范网络 CIDR，最多 64 项。非回环监听必须配置允许来源；本机回环始终允许。网络防火墙需允许对应地址和端口。本协议用于配置允许的可信局域网，IP 允许列表不是应用身份认证，TCP 正文未加密。PID 仍在启动时输入，不自动换绑其他游戏实例。

一个类别一条长期 TCP 连接。Living、Vehicles、GroundLoot、Local 独立采样和发布；Radar 与其他消费者平行直连 Reader。GroundLoot 包含独立拾取物和 GroundGroup 成员；Containers、Projectiles 当前返回 `Unsupported`。

默认业务请求为 Living/车辆/Local 30Hz、地面物资 2Hz。请求支持小数 `(0,144]`，本连接请求同时表示采集需求和发布上限；类别采集需求取所有有效连接最高请求。Actor 类别最高需求决定分类 `+1Hz`、名单 `+2Hz`；Local 独立于 Actor 发现。无 Actor 订阅时分类 1Hz、名单 2Hz。默认最多四个消费者、24 条类别连接，不限制 Actor 数量。

## 记录与状态

每条记录是 **4 字节大端正文长度 + Protobuf ReliableEnvelope**。Hello 正文上限 16KiB，协商后正文上限不超过 16MiB；任意拆读、粘包和短写都必须正确处理。每方向序号从 1 独立严格递增。

1. 发送 Hello，声明消费者实例、订阅身份、类别、请求频率、能力、随机 nonce 和 `clock_contract_version=1`。
2. 精确核对 HelloAccepted 的 nonce、消费者/订阅/类别、会话、能力、采用周期及源时钟结构。
3. 原子安装完整 Baseline，然后发送精确 AppliedAck。增量始终相对**本连接已 ACK 的固定基线**，不能以刚收到的增量替代基线。
4. 按 session/epoch/fence/revision/capture_sequence 单调应用；明确失效须撤销对应旧资格并回 InvalidationAck。重连新会话、新基线；单类别断线不清其他类别，世界代次变化隔离旧世界。
5. GET 不带变更字段；SET 必须携带 request_id 和 expected_pacing_revision。冲突先重新查询，不静默覆盖其他配置。

每连接只有一条正在写的记录和一个最新待发送状态；未开始发送的状态可合并，半写记录不能修改或插入其他记录。慢连接不能阻塞其他连接或采集池。没有采集帧 TTL、记录年龄淘汰或 UDP 路径。心跳、字节无进展、ACK 和明确失效送达分别有协商期限，持续正常传输不会仅因总耗时长被判过期。

## 时间契约

`HelloAccepted.clock` 仅表示 **Reader steady_clock → Reader QPC** 的内部标定，不是跨机映射。消费者可在 Hello.consumer_clock 声明自己的单调计数器（域、类型、频率），回执原样确认；也可不声明并正常接收数据。Windows QPC 域包含本机启动身份；Node hrtime 可使用含进程实例的独立域，类型 `CLOCK_MONOTONIC_COUNTER`、频率 `10^9`。

所有源端采样、验证、目录、分类及发送锚点都属于 Reader 域，绑定原类别批次、序号、session、epoch、revision 和 mapping_revision。接收端组装、解码、发布锚点属于自己的域。**不同域原始计数不能直接相减。** 本机直比必须由接收器本地确认域及频率；源时钟结构、数学窗口和源内先后仍需严格校验，跨机不能用本地时刻判断源样本“在未来”。

| 来源锚点 | 边界 |
|---|---|
| started_qpc | 本批第一次读取前 |
| read_finished_qpc | 最后一次数据读取及必要读取核验后 |
| finished_qpc | CPU 整理和最终提交完成后 |
| send_started_qpc | 完整序列化与 TCP 封装后，首次 send 调用前 |

首次发送锚点通过预留 `optional fixed64` 的八字节原位盖戳写进实际消息；盖戳及调用准备计入发送阶段。短写/暂不可写沿用第一次调用的锚点；新的基线重投是新记录、新发送锚点。缺失保持缺失，不能填零或借用上一批。来源内部标定的 <=1ms 误差要求不等于网络单程精度，也不是帧年龄期限。

### 可选四时间戳探测

在已完成握手的连接发送 ClockProbe，request_id 本会话严格递增，consumer_sent_tick 为可存在的零值。最高 5Hz、每连接一条回复在途；未声明消费者时钟返回 Unsupported，忙碌/超频返回 NotReady，重用 ID 返回 Conflict。探测错误不撤销实体、基线或采集需求。

```text
消费者 t0（封装后首次send前） ──请求──▶ Reader t1（完整记录发现后、解码前）
消费者 t3（完整正文接收后、解码前） ◀──回复── Reader t2（封装后首次send前）
```

ClockProbeReply 回显 t0 和 request_id，携带实际 t1/t2、epoch 和源映射版本；重连/源身份改变后旧探测不得复用。客户端验证回显、身份、各自域内先后，再计算：

```text
净往返秒数 = (t3-t0)/consumer_frequency - (t2-t1)/reader_frequency
```

Reader 的解码、排队、编码开销在 t1/t2 中明确扣除。偏移估计需至少半个净往返的不确定度，再计量化/时钟误差，不能假定上下行对称。四个时间点只支持该探测区间的诊断，不能证明任意未来时刻的漂移上限；延伸映射必须声明漂移假设、探测 ID、样本年龄和不确定度。

未建立跨机映射时，单程传输和跨机端到端延迟显示未知；源内耗时、接收端内耗时仍可精确计算。不能用映射未知、探测失败或样本陈旧拒收其他方面有效的数据，也不能伪造精确单程耗时。

## 人物速度

Living 的人物属性组 `PROPERTY_CHARACTER_MOVEMENT=21` 对应 `CharacterMovementValue`。字段 `velocity` 仍是本批原生运动组件速度；`component` 是实际组件生命周期，`raw_component` 是本批原加密引用。前三项掩码为 `1/2/4`；新增 `world=4` 的掩码为 `8`，完整顶层字段域为 `15`。

`world.velocity` 是当前版本 `Actor::GetVelocity` 等价的身体世界线速度，单位 **cm/s**，XYZ 世界轴；敌我与本地身体用同一规则。`source=1/2/3/4/5` 分别表示运动组件、物理刚体、当前骑乘替换、原生移动基座叠加、原生明确零分支。骑乘直接采用当前载具分支，不把人物残留的运动组件速度再加一次；基座仅在原生条件成立时叠加。`actor/component/carrier` 提供实际生命周期依据；同批直接读取依赖，不等待或借用低频 Vehicles 流，也不使用 LastVehiclePawn。

骑乘按原生 `TslVehicleInterface` 关联，不能只用 WheeledVehicle 子类筛选而漏掉船等对象。基座叠加仅在本版本原生 Vault/移动组件条件成立时采用；Reader 不对任意附着对象自行求和。

此值不代表任意骨骼点因角速度或动画产生的瞬时速度。未知/未支持的原生 getter、缺页、源切换或生命周期失败时保持缺失；不能假定为零、退回旧样本或盲目重加父速度。`nested_samples` 路径 `[4]` 是独立原始观察。空 `world {}` 加失败子观察表示本次已尝试失败；成功的零向量与缺失有区别。增量会明确替换/清除旧值，父组成功不能给子样本续期。

## 本地客户端状态

`CATEGORY_LOCAL=6` 替代原视图类别，入口为 `FrameContext.local`，模型集中在 [model/local.proto](synchronization/v3/model/local.proto)：

```text
FrameContext
├─ groups                   World / Map / Coordinates / Pacing 四组公共终态
└─ local                    仅 Local 类别提供
   ├─ control               本地 Controller、被控对象、身体及跨流引用
   ├─ camera
   │  ├─ source             Controller、CameraManager 的本批生命周期
   │  ├─ pose               最终位置、旋转、FOV、原点及真实读取时间
   │  └─ target             Current/Pending 的原始关联及可选身份
   ├─ current_weapon        仅本地身体的当前原生槽位
   │  └─ weapon
   │     ├─ class_name      当前武器 Actor 类名
   │     ├─ instance        当前武器生命周期
   │     └─ muzzle          当前枪口几何挂点的独立采样
   ├─ spectators            本地身体被观战人数，0与缺失分开
   └─ controller_rotation   本地控制器原生旋转，独立采样与关闭核验
```

Living 不再协商装备属性组，不读取任何人物的装备槽位表；也不再携带本地观战人数。Reader 只在 Local 中读取本地当前选中的一个槽位，`declared_slots` 是原数组边界，不代表复制或传输了备用装备。`active_slot` 的负值或越界、当前槽位真实零引用，分别按原生来源表达为无当前武器；不会改取其他槽位。

控制关系、相机、武器、观战数分别保留终态、字段源时间与关闭区间。死亡、没有本地身体、观战以及相机失败时，各组独立缺失；观战目标不能代替本地身体成为武器/观战计数来源。Local 与 Living 独立采样、独立调频，不建立共同采样屏障；跨流引用暂未解析不能撤销来源值。

本地人物和被控对象引用通过共享生命周期注册表分配 ID。已经登记的 Local 引用只绑定编号，不刷新 Living 的顶层身份采样时间，也不逐批复制完整 Actor 注册表。

## 本地当前武器枪口

路径为 `FrameContext.local.current_weapon.value.weapon.muzzle`。本批武器和枪口的独立观察路径分别为 `[5]`、`[5,4]`，位于 `CurrentWeaponState.nested_samples`；每条证据绑定 Local 批次的 capture_sequence、epoch 与源时钟。切枪、失去本地身体或来源变化后，新状态必须清除旧武器及旧枪口。Reader 不保留旧装备表或旧路径的兼容分支。

`world_transform.translation` 是枪口世界位置（cm），**未加 `source.origin`**，坐标原点规则与骨骼一致；`rotation` 是 `(x,y,z,w)` 的挂点几何世界四元数，`scale` 保留。旋转挂点自身 `+X` 可得到该坐标轴的世界方向，但不能据此宣称它是归零、散布或弹道修正后的真实发射方向。Reader 不写入消费者的弹道假设。

原生 FiringAttachPoint 依次匹配资源 Socket、Skeleton Socket、同名参考骨骼，`binding=1/2/3`。`source` 记录武器 Mesh、骨架、实际姿态组件和可读双缓冲；同时提供挂点相对变换、绑定骨骼组件空间变换、骨骼/姿态真实下标。FName 保留原始及明文索引，未解析字符串不补造 full_name。只有同名骨骼回退使用已证明的单位相对变换；挂点和骨骼均缺失时不回退 Mesh 原点。

挂点失败保留空子结构与失败观察，武器名称等独立字段可以保留；不继承旧位置和朝向。接收端必须核对子观察，不能借当前武器父观察证明其新鲜度。

## 原生控制器旋转

`FrameContext.local.controller_rotation` 是独立可选组，提供本批本地控制器生命周期身份和原生 `Controller::GetControlRotation` 的当前 FRotator（度）。它不是角增量、纯后坐角或实际子弹方向。当前 `.1.1.57` 的原生后坐更新存在经 Pitch/Yaw 输入累加器、视角处理再提交控制器旋转的路径，因此后续控制器样本可能已经包含施加过的后坐输入；不能把另读的后坐目标无条件再次叠加或扣除。尚未证明全部状态下它与最终相机、枪械晃动及射击修饰的完整分解关系。相机、身体或武器失败不应由接收端自行撤销该组；该组失败则清除自己的旧值。

身份与旋转分别携带真实源读取区间，关闭核验另有区间，全部绑定本批 capture_sequence、epoch、QPC 时钟域和映射版本。跨机传输、控制计算和设备生效存在延迟；这些观测时刻不冒充游戏开火时刻，也不承诺控制生效时间。本次没有新增真实发射事件、弹序或开火时钟。

原生枪械方向晃动具有本地计算、同步值与回放值等不同来源；本地计算还混合移动、曲线和武器等条件，不能将同步字段或相机与控制器的角差命名为纯呼吸角。当前协议没有声明纯呼吸角、独立累计后坐角或成功发射事件；枪口挂点几何旋转也不提升为这些能力。原生后坐目标与回正目标属于待处理状态，不能当作本次已经施加的角增量。

此扩展改变正式 descriptor，严格消费者须同步协议与纯消费代码后再配对部署；当前运行版不自动切换。

## 生成

```powershell
.\protocol\generate.ps1 -Language Python
# 使用官方 protoc 生成 C++：
.\protocol\generate.ps1 -Language Cpp -ProtocPath C:\tools\protoc.exe
```

约定编译器 `libprotoc 35.1`，显式生成输出默认在忽略目录 `obj/protocol`。源协议 SHA256、官方描述符和 codec schema 一并核对；修改 .proto 后运行 `-UpdateCodecSchema`，普通 VS 构建通过预生成头校验，不依赖 Python/protoc。消费者须使用相同源协议和官方描述符，不保留旧 UDP/轮转游标/过期样本验证入口。
