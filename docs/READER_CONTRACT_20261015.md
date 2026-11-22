# Reader 2026-10-15 契约同步

本轮随PC0.6.0同步正式Reader工作树协议与纯消费库，仅处理解析兼容，不新增控制功能，不连接Reader或启用鼠标输出。

旧消费者使用CanonicalContract声明人物血量19，并严格检查服务端接受的类别契约，因此与删除血量后的上游存在实际冲突；不能仅在业务筛选中忽略血量。已从上游同步10份proto、descriptor与30份纯消费源码，并更新冻结清单及检查工具。没有修改上游源码或手改生成schema。

来源G:/pubg-hyperv-memory-reader；正式生成目录R:/pubg/analysis/reader/manual-update-2610.1.1.57-20261015/protocol-generated，libprotoc35.1。descriptor SHA256为D48540160AF6D5A68F39BAF8256ED76731F226B8425ECD77B6465D91CBBA7EED。此前descriptor与源码备份于out/qml-ui-20261015/reader-sync。

- CharacterHealthValue、PropertyGroup19和PropertyValue.character_health19均删除；19不重映射到骨骼或运动，严格解析拒绝旧oneof19。
- CharacterSkeleton20、CharacterMovement21、独立View及跨机时钟契约保持。现有原生速度/零速度SET/失败CLEAR二进制夹具继续通过。
- WeaponValue新增class_name4(NameValue)、source_identity5(InstanceIdentity)，保留在CharacterEquipment → members.slots[].weapon中。纯消费层接受有本批独立nested_sample的名称，校验source_identity地址与object_address一致；缺身份、地址不一致、缺独立证据和错误capture均拒绝。
- active_slot0保持合法，类名失败保持缺失，结构性关联可保留。本项目未新增武器业务选择、武器配置或按武器控制；不缓存旧武器名称用于算法。

Debug/Release通过新契约断言及原协议/投影/时钟回归，官方Python protobuf确认运动21字段与零值来源；两配置五场景TCP回环验证正常/重连、无时钟、无心跳、超大记录和停滞片段。验证是本机合成记录与回环，不构成上游新版部署或实机联调证据。

## 上游就绪通知

2026-10-15，Reader项目通知新版已部署远端并绑定目标PID21816，EXE SHA256 AAA516B234C906BFD45DAA71F121F6C33EADDB8DB1F5BAB122CF5FC9274036AF。descriptor与本项目冻结版本相同；上游报告Living/车辆/View正常，当前武器名称仍在现场核对。后续PlayerArray和接收缓冲首次驻留修正无协议变动，消费者无需额外更改。本项目仅复核冻结契约，没有独立远端联调或鼠标输入，不能将上游通知扩张为本项目实机验收。

## 后续同步：spectated_count（已完成，最终接收端PC0.7.0）

正式生成物到齐后已同步人物状态组3的optional uint32 spectated_count=7，位64，严格允许掩码127与原生非负int32范围。合法0与缺失区分，不继承旧值；未新增GUI或控制业务。当前descriptor升级为746609C5BD303C01FCE7F2F775A42C87C9405CFB881FB991F28E7550B593EF7D，上文D485为该字段加入前的历史契约。详见[观战人数同步](READER_SPECTATORS.md)。
