# CS2 资源 Hitbox 与血量显示

2026-10-26，PC2.6.0与Reader 3d66姿态契约已完成配对换版。

## 范围

本次只实现 CS2 资源定义的 Hitbox 和已有正式 HP 字段展示。PUBG 不采集、不下发人物血量，不改其协议。CS2 Aimbot/RCS、原生后坐求值和鼠标输出资格保持原有行为。

## 资源核验

本机 build 14192 的80个已核验人物主体模型全部共用一个 `cstrike` 集合、19个胶囊体。343个嵌入 mesh 的集合和原始定义摘要一致；模型没有外部 mesh 引用。角色外观不参与命中体尺寸估算。

`data/games/cs2/hitboxes/agents-14192.json`保存逐模型原始资源SHA256、mesh/group/LOD映射、原始定义摘要及去重后的形状。`tools/assets/cs2_hitboxes.py`通过固定摘要的Source 2 Viewer CLI 20.1提取 DATA / MDAT / CTRL；原始模型和文本只在指定 scratch 目录，运行包不包含人物网格、纹理或资源解析工具。工具按源码同样的大小写不敏感规则唯一映射骨名，遇歧义或未知形状直接失败；胶囊的a/b是轴端点，不能排序为AABB。

生成头 `src/games/cs2/hitboxes/generated/agents_14192.hpp`将小型共享定义编译进core。80个模型引用同一份19形状数组和基础表面模板，个体只更新当前骨姿态。模型/构建/骨架profile都须匹配。`tests/assets`检查解析边界与头文件可复现性，改动资源或生成头触发该检查，无需扫描地图包。

首个头部胶囊真实数据：a=(-1,1.8,0)，b=(3.5,0.2,0)，radius=4.3，绑定原生骨7。19个命中体所需骨为1–7、9–11、13–15、17–22，均在当前21主干内。

参考实现为 [VRF 20.1 Hitbox](https://github.com/ValveResourceFormat/ValveResourceFormat/blob/20.1/ValveResourceFormat/Resource/ResourceTypes/ModelData/Hitbox.cs) 和 [HitboxSetSceneNode](https://github.com/ValveResourceFormat/ValveResourceFormat/blob/20.1/Renderer/Renderer/SceneNodes/HitboxSetSceneNode.cs)。

## 姿态与资格

Reader核验原生骨数组，每骨同源 `world_rotation`（xyzw四元数）与累计 `world_uniform_scale` 已接入。正Z90°把+X变为+Y，累计有符号缩放只应用一次，0/负值保留，不能再乘实体scale。近单位q保留原值，以原生叉积公式计算，不隐式归一化。正常来源缺失只跳过对应命中体，不补旧帧，不降低原骨点或Aimbot/RCS的独立资格；非法wire按契约严格拒绝。

唯一资源集合不能证明运行态一定有命中体：引擎仍以MeshGroupMask筛选mesh。`Skeleton.native_hitbox_set_available=6`只有本批明确true才绘制；false与缺失分开。`BonePoint.world_rotation=4`、optional `world_uniform_scale=5`成组出现；Hello字段13声明能力，逐点仍检查存在性。四元数有限且abs(norm²−1)≤.001，scale有限。当前冻结schema为3d66db7d1b9b126ef27c6020ff944d9eeb466a7109c39d9d5bc26b1e57e0d457，旧1288不并行接受。

## 原生绘制

游戏内的 `hitboxes` 模块负责资源和姿态。纯几何生成确定性表面模板，游戏投影将其转换成通用clip三角形。通用D3D painter不认识CS2、Hitbox骨号或Reader，独立深度纹理处理人体部位前后遮挡；输出GPU纹理由已有D2D画布合成，不向JS传逐帧几何或像素。

表面网格按当前不可变快照、选项和视口缓存。显示选项为off/solid/wireframe，默认off，与原骨架开关独立。姿态缺失、资格撤销、切换或断线不会沿用上批实体。所有世界尺寸遵循相机投影，文字仍用既有缓和缩放。

CS2掩体扩展优先完成原有骨点/骨段采样，再用同一有界预算采样命中体表面。表面顶点与中心存在不同结果时有限细分，未确定部分保留原色。附件绑定原scope/capture和原始快照，不能贴到更新的姿态。它仍是有限分辨率的静态物理可见性，不能称为逐像素边缘或服务器回溯命中判定。

## 血量

CS2已有EntityState.health/max_health。`cs2ShowHealth`默认true，传入通用展示图元，使用当前值显示HP数字；零值保留，缺失不显示假数，最大血量缺失时不补100。纯展示不参与目标选择、死亡门控或RCS。用户已明确撤回PUBG血量要求，PUBG Reader请求已取消。

## 已验证与保留范围

- 64官方记录的精确摘要、上游独立932检查，以及消费者106项/27姿态记录/9非法wire/五批生命周期通过。
- Debug/Release原生all、资源解析8项、命中体544项（含真实无窗口WARP深度/DPI/GPU合成）、HP存在性与既有控制回归通过。
- 333项Web/真实stdio/TCP、类型/lint、13架构/617源码、隔离隐藏Electron开关/恢复默认/暂停零查询通过。
- 正式13202只读3秒89帧有效，首批210点/210姿态、10个集合true、190个资源可还原胶囊，10份HP/maxHP及真实0保留；没有自动输入。
- 已配套换版并保留旧程序与149份原数据。实际前台观感、动态场景真值与服务器回溯命中未自动验证；只读probe不宣称GPU完成时间。证据见PROJECT_MEMORY和out/hitbox-investigation。
