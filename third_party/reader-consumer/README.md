# Reader 消费协议源码快照

## 当前候选：Consumer v3 / PC2.0.3

2026-10-22，按 Reader 正式 `SDK_MANIFEST.json` SHA256 `af09ae0f45189ff7b544e1ca96c3c0daf663e43b8de0c1ea751d5d063b4e463b` 导入完整99文件；86份src、12份proto及protocol README保持上游原字节。descriptor为 `4e5f26fed497f86a37d686df443be54d42dcc5e1eeeae79143a19f209e5f6f92`。只编译17个client CPP，11个provider CPP不参与本项目编译，生成项与对象路径见client-sources.props；不能恢复全目录CPP通配。

默认Consumer/profile1/features0，正式业务State/Receiver/19与20号payload；不把业务token填进Native地址/serial。Frame保留独立源区间与失效规则，骨骼使用语义role/parent_role。Debug原生模型仅用于既有诊断夹具和文件回放，TCP入口拒绝v2。旧v2丰富记录仅在recorded_codec文件读取入口显式按同一值结构映射为Debug验证，不开放在线兼容。

SDK源、声明、编译闭包、descriptor及公共协议镜像由tools/check-reader-contract.ps1固定核验；IMPORT_RECEIPT记录导入范围。实际回归/待部署状态见项目记忆和docs/READER_CONSUMER_V3.md。以下“当前”均为历史阶段，不代表当前编译SDK或现场版本。

## 历史冻结：24dba290 / Reader 2610.1.1.71（PC1.2.8当时已部署）

2026-10-21，从正式`reader-consumer-2610.1.1.71-24dba290-20261021.zip`完整交接57文件，41纯消费源码/15CPP/10proto按上游原字节保持。ZIP SHA256 `d5a1fd98973acca9ed2d782161c62d081f9a1d19e9d8740b427f3e6acc4a591e`；descriptor仍`24dba29046b068250a53544cf0648170ac0ff4bd82a631462e9053b00e3fd94c`。相对上轮只有local_validation.cpp两处精确profile `.65`→`.71`，其余源码与proto不变。

当前双清单SHA由本仓库冻结检查固定，完整载荷、源码和15CPP列表交叉核验；不只改清单掩盖源码修改。84项真实Encode/DecodeReliable→Session::accept、15官方Protobuf往返/10负例、10状态真实TCP/桥回环及两配置all通过。SDK wire仅接受`.71`方向，应用层保留`.65`状态谓词不能解释为旧wire兼容。没有改Reader字段、部署远端或伪装profile。

实际导入/发布范围见[兼容说明](../../docs/NATIVE_PROFILE_COMPATIBILITY.md)及本目录`IMPORT_RECEIPT.json`。自有README/导入收据不冒充上游冻结文件。以下各节均为历史快照，旧数量、descriptor、部署状态和“当前”措辞只适用于其当时日期。

## 历史冻结：62f87cba原生瞄准观测（当时PC1.1.3候选）

2026-10-16，descriptor `62f87cba68fbf38ab35828e75becf6a4d850de1ec03c965f243f06d14aa7a26a`。来源为Reader `.65`正式消费包，ZIP SHA256 `688b65562957f53c0b7356250a587e1558d19088852e2379ed9fb5eab976cf8b`。当前41份纯消费文件、15CPP、10proto均保留上游字节；相对9bef移除两个未引用的movement/world_velocity头文件，未向消费闭包引入采集器、偏移或解释器。`SOURCE_MANIFEST.json`、正式proto/schema与构建pin同步冻结。

Local新增独立native_holding、native_sway和aim_basis；本候选按原生语义投影身份、来源与合法失败，仅诊断。缺失不补0，混合sway不冒充纯呼吸，已归零basis不再次叠加sway/remaining recoil。当前未证明鼠标输入到该方向的响应映射，未新增控制补偿。详见[当前候选](../../docs/READER_NATIVE_AIM.md)；正式运行仍1.1.2/9bef，等待配对切换，不用开发包覆盖线上兼容性结论。

## 历史冻结：9bef3ef6原生武器遥测（PC1.1.1候选，随后1.1.2上线）

2026-10-16，完整descriptor SHA256=`9BEF3EF6E5B3EAD866080D440327343EDBB69487BD3E0F6A4E408F128F2B56AE`；当前43份纯消费文件、15CPP、10proto按上游字节冻结。来源包、ZIP SHA及语义见[原生遥测契约](../../docs/READER_NATIVE_TELEMETRY.md)。新增可选原生控制器旋转及当前武器独立recoil_targets/trajectory_generation，均仅作原生遥测；不提供纯呼吸角、已施加纯后坐角或持续开火bool。旧严格消费者拒绝新字段，Reader窗口统一协调切换，当前本地构建不等于线上已经更新。

当前`SOURCE_MANIFEST.json`、codec标记和`tools/check-reader-contract.ps1`均对应上述descriptor。源码保持上游原样，不包含采集器；新增官方Protobuf核验见`tests/reader_native_fixtures.py`，正常运行不依赖Python。

## 历史冻结说明（下述数量、descriptor及字段不表示当前版本）

来源：2026-10-16 Reader正式世界速度/枪口消费ZIP，完整交接见[契约说明](../../docs/READER_CONTRACT_20261016.md)。本目录含纯模型、严格codec、校验及ReceiverSession所需41份文件（14个CPP），不含采集器/服务端。按完整SHA清单冻结，不将工作树冒称提交或发行版。

`SOURCE_MANIFEST.json` 固定全部来源文件。源码保持上游原样；尤其采用上游新增的 `ReceiverOptions.source_clock_is_local=false`，源时序核验与本地提交时钟核验分开，没有通过伪造本地时间放宽校验。

协议入口在本仓库`protocol/reader/synchronization/v2/synchronization.proto`，Envelope2、clock1。libprotoc35.1完整descriptor SHA256=`196947A2B345C89D6505E4FE37C3F33E4535E65642422721C0F5B407CF1028E3`；schema.hpp从同一descriptor生成，不依赖C++ protobuf runtime。

CharacterMovement组21允许掩码15：原生velocity/component/raw_component保持独立，新增world4为完整身体世界速度cm/s，骑乘/基座贡献已计入。独立[4]观察有效才投影，不再假定父速度0；显式0合法，缺失/CLEAR不继承。WeaponValue新增muzzle6，按当前槽位和独立数组路径证明，详见[时间补偿](../../docs/TIME_COMPENSATION.md)及交接。冻结不代表远端已升级或实机验收。

构建通过本仓库相对路径完成，不依赖来源仓库仍存在。`tools/check-reader-contract.ps1` 验证 descriptor、10 个 proto 和全部源码快照；更新时须从正式上游重新核对完整依赖和契约，再更新清单并重跑协议与回环测试。不可只改清单掩盖不兼容修改。

独立兼容性测试 `tests/reader_loopback.py` 使用官方 Python protobuf runtime 动态加载冻结 descriptor，对 C++ 夹具解码、重编码并运行本机 TCP 对端。本轮测试 runtime 为 7.36.2；测试工具环境可外部提供，正常客户端不依赖 Python。

此前删除血量19，保留骨骼20/运动21；class_name4/source_identity5及独立武器身份校验继续保留。PC0.9.0通过当前武器真实class_name选择参考弹道，不修改上游协议语义。先前交接见[2026-10-15记录](../../docs/READER_CONTRACT_20261015.md)。

PC0.6.1同步CharacterStateValue.spectated_count7，组3新增位64，允许掩码127。原生非负int32范围，0保持存在，缺失/CLEAR不继承；业务投影不使用此值控制鼠标。完整验证见[观战人数契约](../../docs/READER_SPECTATORS.md)。
