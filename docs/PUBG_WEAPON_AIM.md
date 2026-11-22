# PUBG 枪械分类瞄准与乘车校验

PC2.0.5，2026-10-23。

## 分类部位

Aimbot 页保留原 `part` 作为默认部位，增加八类独立选择：SR、DMR、AR、SMG、LMG、霰弹枪、手枪、其他已识别武器。每项可以跟随默认（-1），或选择头、颈、胸、腹、骨盆（0–4）。参数按工作区保存；初始全部跟随默认，因此不会把已有用户的部位改回头部。设置变更沿用现有代次撤销规则，松开侧键再按。

对应键为 `partSR/partDMR/partAR/partSMG/partLMG/partShotgun/partHandgun/partOther`。旧工作区缺少这些新增键时由 PUBG GameConfiguration 显式允许，规范化后采用 -1；存在的非法值仍拒绝，不放宽其他必需参数。通用工作区层不包含游戏字段白名单。

`model/weapon_aim.hpp` 定义分类和部位策略，`pipeline/weapon_aim.cpp` 根据本批完整归属和武器身份解析有效部位。实际控制、ESP 预选和说明界面共用同一解析；Web 不按名称猜测。当前类别和有效部位由原生状态推送。持枪缺失、身份不符、资料冲突或未知分类时使用默认，不能沿用上一把枪的类别；弹道资料的独立资格检查继续生效。

分类来自现有本地 `data/games/pubg/weapons/2026-10-15/pubg-gs.json` 的 category 字段，以严格资料 profile_id 关联全部83条记录。`tools/generate-weapon-categories.py` 离线生成 `data/weapon_categories.inc`，`--check` 核对一致性，不联网、不改弹道资料和冻结 SDK。新增资料ID在映射补齐前采用默认。SR 是原资料的狙击步枪分类，也包括 Lynx AMR 和 Win94，不声称每款都为栓动；Launcher/Crossbow/Melee/Special 合入 Other，不因此给无有效弹道的物品输出资格。

## 相机与目标的同刻筛选

原选人路径只将敌方骨点按速度外推到决策时刻，却仍与原始旧相机位置求角差。己方快速乘车时可能产生虚假的角差或距离，进而把前方人物判为超出范围。

`predictCameraPosition` 现在复用独立 Local 相机时间、Living 本地身体完整世界速度和用户的 `livingLocalSkew` 上限。选人和弹道共用它；选人中目标外推成功且相机也合格时，比较同一决策时刻的双方位置。缺少己方速度或时间时保留既有选人后备，最终弹道仍按缺失拒绝；没有扩大角锥、伪造零速度或改成骨点差分。

Reader 骑乘速度 source=3 已包含载具贡献，只用一次；不另订 Vehicles 流、不再加一次车速。相机平移用于对齐起点，仍按用户已有约定不向飞行中的子弹加入射手速度继承。

## 本次真实乘车故障的范围

用户进一步明确：自己乘车时提示“等待自己完整身体世界速度”；敌人乘车尚无陪测，不能把两种场景都写成已现场复现。

用户回复“已经乘车”后，使用现有正式只读 CLI 抓取4秒，双流 CURRENT。固定基线与增量经官方 Python protobuf 独立重建：Reader epoch45，Local 身体1424、控制器4430，Living capture182848，Local201306。18名人物中仅己方1424的完整世界速度缺失；Movement21 为 Partial，有合法零向量 component_velocity，world 子项缺失，source path[4] wire status=4（依赖阻断），非复用。故现有 SelfVelocityMissing 符合正式数据，不是客户端漏接 source=3。

Reader 窗口只读审查发现现有载具速度 getter 的静态返回类型与向量分支用途不一致，仍需其内部现场 Issue 才能将本次故障归到具体阶段。本窗口没有修改 Reader 字段、协议、采集代码或远端，也没有把残留组件零向量当作载具世界速度。相机对齐修复不等于已修复这个上游缺失，不能承诺乘车实机已可用。

本轮外部取证目录：`G:\mousebox-analysis\weapon-categories-vehicle-20261023`。`reader-riding-confirmed` 为故障抓包，`reader-riding-confirmed-wire-summary.json` 为独立解码摘要；较早 `reader-riding` 实际是步行 source1，仅作对照，不能冒称乘车证据。
