# VIEW_NO_TRACE 与当前物品：最终 24db 消费合同

2026-10-16。Reader 最终描述符 `24dba29046b068250a53544cf0648170ac0ff4bd82a631462e9053b00e3fd94c` 取代历史候选 823188e3 与旧生产 62f87cba。冻结来源为 Reader 的 `bool-view-contract/consumer-contract-24dba290`，压缩包 SHA256 `3724c49e0d8b4f02a462115ed6a47323e66b3fa02fc1580ff204144c9d05b93a`。本仓库逐 SHA 核对 41 份消费源码（15 CPP）、10 proto 与描述符，保留旧合同备份。证据在 R:/pubg/analysis/aimbot/contract-24db-final-20261016。

## 当前物品

`CurrentWeaponValue.has_current_item=7` 是有存在性的布尔值，字段来源 origin=2，有独立采样时间和本批关闭核验。

- true：本批选择了非零、生命周期有效的物品，可为枪械、近战或投掷物；不是可开火、实际射击或动画完成信号。
- false：本批核验原生 getter 空槽或无当前有效槽。读取失败、对象未知、证据复用、其他身体或非当前批次保持 null，不能转成 false。
- 非枪械的 muzzle 失败不代表空手；完整当前物品身份仍保留，枪械资料按真实 class_name 单独匹配，不按名字猜类型。
- 投影 `Frame.currentItem` 与 `reader_telemetry.current_item` 保留原状态、字段来源与 presence_sample；运行快照 `hasCurrentItem` 和概览“当前物品”使用同一值。ActiveWeapon 还需要原有选择槽、所有者、对象和嵌套采样核验，不凭 true 单独授权控制。

## 无射线视线观察

- branch=2、stage=1、精确 mask=0x782b，只允许 1/2/4/6/12/13/14/15。
- profile=`2610.1.1.65/view-no-trace-v1`，native_zeroing_applied 必须明确存在且为 false。
- 不存在 Mesh、轨迹配置、归零距离/角度或积分字段；均保留 null。
- 方向为明确不做物理 trace 分支的 adjusted-aim 世界单位 forward。它仅观察，不能复用 BARREL 已归零阶段的 D_launch 比较或输入映射；资格函数与比较仍拒绝 branch2，旧活动反馈不能标成该分支当前控制。
- BARREL=1 的 profile、原生归零和资格规则保持；两种方向均不额外叠混合 sway 或 remaining recoil，不从 forward 伪造 roll。
- 需要物理 trace 的分支若仍返回 AIM_BASIS_TRACE_REQUIRED 就是缺失，不用相机或枪口方向代填。

## 验证范围

Debug 构建和 all 通过：172 项原生遥测投影（含当前物品 true/false/null、错误来源、最终 view 线包及控制拒绝）、90 应用、14 生命周期，以及原有控制与弹道。官方 Protobuf 与真实 C++ 对 15 个有效/10 个无效最终线包双向互验通过；旧 62f 严格消费者实际拒绝 Unknown protobuf field。真实后台/管道覆盖 10 次独立 Local 状态、空手/非枪械/恢复与 view 观察；没有设备租约或输入输出。63 Web 测试通过。

旧 Local/遥测夹具保留在原目录；新 -24db 目录明确记录为合成合同迁移，带 MIGRATION.json、原 SHA 和新 SHA，不将迁移样本冒充新的游戏实流。Release、隔离打包、正式安装及 Reader 配对实流的最终结果，以随后部署收据和 PROJECT_MEMORY 最新记录为准。

最终交付：Debug/Release全回归及上述协议、真实后台管道和网络检查均通过。PC1.1.5已安装并21:28最小化重开，main35352/native50308，默认不自动连接。已安装Native对新生产Reader独立验证50快照：body/item/holding/basis各48有效，sway47有效/1缺失正确清除后恢复，当前M416/BARREL1；官方Protobuf另验原包与完整自己身份唯一匹配。无输出租约、串口或实际鼠标注入，数据接入结果不代表实机控制效果已验证。完整版本、哈希、保留配置与范围见PROJECT_MEMORY最新段和同R目录handoff.json。
