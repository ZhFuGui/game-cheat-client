# Reader 原生屏息、混合晃动与散布前方向

**PC1.1.4更新：** 已基于本文冻结的62f合同实现[原生方向整体反馈](NATIVE_DIRECTION_CONTROL.md)。方向与独立Controller轴合格时可于下次按下采用明确的局部姿态/校准近似模型；不等纯呼吸，不叠sway、相机残差或第二次归零。输入映射仍未游戏实测。下方PC1.1.3“未参与控制”“映射待实现”和19:49生产限制仅记录当时范围，不能当作新实现或现时Reader实流结论；本轮实流和换版结果看1.1.4最终收据。

2026-10-16。本轮按Reader正式冻结交付准备本地候选，**未部署**，不自动关闭或替换用户正在运行的PC1.1.2。Reader生产端接入与最终配对切换由Reader `main`协调，本文的本地完成度不表示远端字段已经上线。

## 冻结与来源

- descriptor：`62f87cba68fbf38ab35828e75becf6a4d850de1ec03c965f243f06d14aa7a26a`。
- 上游ZIP：`R:/pubg/analysis/reader/native-aim-2610.1.1.65-20261016/contract/reader-consumer-contract-62f87cba.zip`。
- ZIP SHA256：`688b65562957f53c0b7356250a587e1558d19088852e2379ed9fb5eab976cf8b`。
- 56包文件，41份纯消费源码文件、15个CPP、10份proto。所有消费源码和正式协议保持上游字节，逐文件SHA冻结；相对9bef移除了不再属于依赖闭包的两个未引用头文件，已有前件备份。
- 初期开发证据、构建/验证日志和首次候选：`R:/pubg/analysis/aimbot/native-aim-62f87cba-20261016/`。最后原生增量晚于该旧包，不能继续把首次候选作为最终一致包。
- 最终验证及一致候选：`R:/pubg/analysis/aimbot/native-aim-62f87cba-final-20261016-194242/`；归档：`L:/PUBG/出射方向分析/aimbot-native-62f87cba-final-20261016-194242/`，以该目录`handoff.json`和交付清单为准。

不改固件、目标游戏、hyper-V或原控制/弹道模型，不提交或推送。根README保持原内容，正式VS仍core/native两工程、本地增加tests。

## 三组独立观测

同一`CATEGORY_LOCAL / LocalState`新增可选字段6、7、8，内部组号12、13、14；不新增采集端口、业务类别或高频JS通路。

| 组 | 本地模型与诊断 | 独立来源与边界 |
| --- | --- | --- |
| `native_holding` | `Frame.nativeHolding`；accepted、blend、身体/动作组件完整身份 | 两个动态字段分别保留源区间，false/0保留；Partial可仅有其中之一。原生接受状态与键盘意图不同，blend不是通用抑制率 |
| `native_sway` | `Frame.nativeSway`；原生混合三轴FRotator及身体/武器身份 | 单位度，rotation来源为实际解释origin=2，覆盖真实输入。可能包含运动、姿态、配件及屏息，不改名纯呼吸，不额外加入basis |
| `aim_basis` | `Frame.nativeAimBasis`；forward、五项完整身份、原生归零、积分信息与branch/stage/profile | 当前已证barrel，`post_branch_pre_spread`，原生归零已施加；world forward没有位置/roll，不是未来单颗随机方向 |

**生产可用性补充（19:49）：** Reader当前main确认`native_holding`可采，`aim_basis`仅支持已核验barrel＋native zeroing后/pre-spread；`native_sway`虽已进入正式合同，但当前producer返回独立`NoReference / NATIVE_SWAY_DEPENDENCIES`，没有rotation值。本窗口已只读核对上游`sway.cpp`相应失败路径。上表说明消费者支持的模型，不表示三组在实际游戏状态下都有有效值；夹具中的零/非零sway也不是生产实采。消费端应照常保留失败与空值，不以0、相机差或别的组代填，不让此独立失败污染holding/basis。未闭合sway不阻塞本次协议消费和包装收尾。

五项basis身份为身体、当前武器、Mesh、选定GunData、轨迹配置，分别保留真实来源。必须存在完整身份、当前字段证明、关闭核验、方向、归零值与明确branch/stage/applied/profile才投影方向；高级积分三项共同存在，解析分支不制造0步，reached=false是实际结果。独立身份可在相机或其他Local组暂缺时保留原始观测；当前身体/武器关联缺失不能授予控制或对比资格，明确身份冲突由严格合同拒收。

每个`NativeObservation`保留原Reader终态、本批capture、关闭核验区间及逐字段源时间/origin。合法ReadFailed、DependencyBlocked、CLEAR或缺组撤销各自值，不能借新相机/武器时间刷新或沿用上批值，也不污染其他独立组。畸形协议仍严格拒绝。

## 展示与方向比较

Aimbot → 算法来源 → 仅原生遥测：原35项扩为38项，仍5组，新增屏息/混合晃动/方向三项。显示真实零、六位角度、各动态源年龄，以及原生profile、归零和可选积分结果；终态与未提供分开。所有原始大整数时间、capture和地址用字符串，不经浮点截断。

`reader_telemetry`新增`native_holding`、`native_sway`和`aim_basis`对象，保留`field_sources`与`verified_start_ns/verified_finish_ns`；`verified_pre_spread_direction`在有已证原始方向时提供，不等于控制资格。未知实际开火、纯呼吸和独立已施加后坐角仍为null，已有轨迹计数仍是原始uint32。

可用时，`native_aim_comparison`用当前同武器的`BallisticSolution.launchDirection`与该已加归零basis计算世界夹角；不用已经扣过用户等效归零的`aimDirection`，不再次扣用户零点，不加入sway或剩余后坐目标。检查当前Reader上下文、同身体/武器、已支持`.65/barrel-zeroing-v1`规则及各必需字段自己的可靠时钟资格。未知profile保留原始观测但不给已支持比较资格。

夹角表示**源时刻观察方向与决策解的只读差异**，没有做新的异时角度外推，不冒称控制生效时刻误差：`time_aligned=false`、`angular_extrapolation_applied=false`。没有可靠时间时原始方向仍可显示，夹角保持不可用。沿用年龄含误差≤250ms、时钟误差≤25ms、探测年龄≤3000ms；新组不能借相机或发送时间满足资格。

## 当前控制边界

**不以获得“纯呼吸角”为接入前提。** 原生混合方向若与输入映射闭合，可以整体反馈，不必拆分纯分量；本候选已接入上述正式观测与诊断。

仍缺少的是现有鼠标输入轴/响应到barrel方向的已证传递关系：当前`angleTo`用完整相机pitch/yaw/roll建立坐标架，现有校准证明的是相机每计数角度。单一forward不能自动提供roll，也不能证明相机校准对含动画/挂点的原生方向完全适用。因此本轮`input_mapping_verified=false`、`used_for_control=false`，不伪造roll=0或将世界夹角直接换鼠标计数。这个具体缺口已向Reader回报，不以未经证明的补偿凑功能。

这层映射及离线验证是本项目的剩余接入工作，不意味着必须让Reader再找一个纯呼吸角或单独Roll字段。可以依据已有完整控制器/相机姿态与原生方向构造明确的输入模型并验证；确需额外上游事实时再指出具体来源和应用关系，不能把尚未实现的控制接入笼统表述为“协议缺数据”。

实际输出保持已有最终相机闭环与通用外扰预测；ESP、目标可视筛选与用户指定的相机弹道起点保持。没有新增独立后坐/呼吸补偿，也不要求actual_firing=true才能观察未开火晃动。后续映射充分时应独立建NativeAimReference、重置预测并按同阶段D_launch计算，不能覆盖显示相机或与旧相机残差双计。

## 验证入口

- `mousebox-tests --suite reader`包含原生投影、独立来源、身份/零/Partial、时钟年龄、阶段/profile、原生归零不重复及真实Receiver CLEAR回归。
- `tests/reader_native_aim_fixtures.py --exe <tests.exe> --output <目录>`：11份上游正式夹具用官方Protobuf重新序列化并与实际C++语义输出比较，5份负例实际拒绝；可用`--old-exe`证明旧严格消费者拒收新字段。
- `tests/reader_native_aim_runtime.py --exe <native.exe> --output <目录> [--fixture-dir <本次导出>]`：真实Backend/管道消费可用、basis失效、holding/sway失败、可选组消失及恢复五种Local状态，验证清除与代次稳定，始终不打开串口或获得输出会话。
- 两配置原生all及原有五类TCP回环/27批Backend上下文回归继续保留；Web/桌面实际管道、类型、lint和隔离发布包验证另存日志。

夹具与运行时回环均使用官方或明确合成数据，不代表游戏实采、呼吸抑制率、1000Hz设备吞吐或实战效果。候选完成收据记录实际结果及包哈希；等待Reader完成生产端并发出配对切换指示后，才另行升级。

## 最终验证与包装（2026-10-16 19:47）

最后源码重新构建后，两配置all通过：各127项新原生瞄准检查、85应用、14生命周期及原有控制/弹道/Reader/ESP等回归。两配置各11官方夹具互验/5负例、5种Local真实Backend状态、5类TCP回环和27批上下文回归通过；另有59项Web/桌面/真实管道、typecheck/lint、桌面构建与原遥测夹具兼容检查。旧严格消费者拒收仍引用前期实际证据，本轮未重复执行，官方最终报告相应值为null。

最新Release、staging、候选包与ZIP内native SHA256统一为`04F7F68B2E1E27BF6FBA520887C17ADC2627750A2356438957F4969367AD1EF1`。安装包与zip已生成，ZIP不含Qt DLL。隐藏隔离启动1.1.3、6页、原生子窗口、Renderer崩溃撤资格与退出通过，visible=false/focused=false，没有Reader/盒子连接及残留候选进程；导航计时仅代表隐藏DOM布局。

本轮核对328份控制/runtime/固件来源SHA未变，正式安装包的1.1.2 App/native SHA也保持。最终交接目录见上方来源，正式换版和远端配对尚未执行。该候选完成了协议消费与包装，**没有启用新的方向控制或完成实机补偿验收**。
