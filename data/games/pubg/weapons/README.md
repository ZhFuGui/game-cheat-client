# 武器资料与可替换数据包

资料版本 `weapon-reference-20261015`，数据包合同 v1；PC 程序仍为 0.8.1。本目录供后续弹道模块使用，本次没有把弹道解算或自动开火接入实际控制。

最新实施核验已按用户的条件停止：现有Reader与消费端契约一致，但还缺真实发射变换、完整身体世界速度、实际重力/衰减及速度继承规则。资料和数据接口保留，停止原因见[设计状态](../../docs/BALLISTIC_AIM_DESIGN.md)。归零距离已由用户确定为全局手动选择，默认100米，不要求Reader先补当前归零数值；尚缺的瞄准/枪口几何关系与该数值分开处理。

最新分工：Reader处理真实枪口发射变换与敌我完整世界速度；本项目已完成[弹道规则公开调研v1](../../docs/BALLISTICS_RULES_RESEARCH.md)，对应证据索引为`research/2026-10-15/rules-evidence.json`。已核对公开曲线计算器及历史载具实验，但精确有效重力、游戏子步和当前继承规则仍未查实；该研究索引不属于冻结武器快照，不改变`reference_only`状态或恢复实机弹道实现。

当前索引：[全部83条](2026-10-15/catalog.md)。其中包括近战、特殊装备、发射器、活动变体，不能把83解释成83种普通枪械。60条有初速，54条有网站公布的弹速曲线。类别：AR15、DMR9、SR7、LMG4、SMG10、Shotgun5、Handgun11、Crossbow1、Launcher2、Melee5、Special14。

| 文件 | 用途 |
|---|---|
| `2026-10-15/profiles.json` | 后续算法侧的统一数据包；83个独立武器定义，参数与来源结构分离 |
| `profile-pack.schema.json` | JSON Schema Draft2020-12；定义版本、单位、武器绑定、模型标识、参数、出处 |
| `2026-10-15/pubg-gs.json` | 完整来源目录与83个详情页的结构化字段：初速、弹速/后坐力曲线、射程、散布、操控、偏移、配件兼容、文件身份等；附配件/弹药/命中区域/规则总表 |
| `2026-10-15/pubgplanet.json` | 54种武器的另一来源资料，含最新记录及页面公开历史、阻力字段、记录ID与发布方报告的源文件摘要 |
| `2026-10-15/official-names.json` | 官方PUBG API资产仓库的物品/伤害来源名称；固定提交，不包含弹道规则 |
| `2026-10-15/aliases.json` | 精确名称与ID映射，保留歧义；不会遇到重名就任选一把 |
| `2026-10-15/responses.json`、`raw/*.gz` | 逐URL抓取时间、响应SHA256与压缩原件；未执行网页脚本、下载图片或接触游戏 |
| `2026-10-15/checks.json`、`manifest.json` | 数量、缺项、来源差异、包内文件校验信息 |

主来源为 [pubg.gs](https://pubg.gs/about/)，页面自报Update43.2、Steam Build25901960，2026-10-14提取。对照来源 [PUBG Planet](https://pubgplanet.com/wiki/weapons) 最新公开记录为Build25298702，2026-09-14；保留在独立文件，不覆盖主来源。名称来自 [PUBG官方API资源](https://github.com/pubg/api-assets/tree/32b13b51128b8d8909ae5e77f3b833e01230b24d)，固定提交32b13b51128b8d8909ae5e77f3b833e01230b24d。不同来源、不同版本的参数不逐字段拼成一个假定的实机模型。

`profiles.json`是与未来算法之间的稳定入口。`model.id`标识有版本的模型适配器，`model.parameters`装该模型参数；Reader以后提供新数值时换数据包和revision即可。模型本身规则变化时新增适配器版本。整个模型及配件/弹种/模式适用条件一起替换，一次计算固定一个不可变revision，不在一次飞行积分中途切数据。

来源接入顺序设计为：本次实际武器的Reader实时参数 → Reader分析出的匹配模型包 → 明确选择的本地校准模型 → 公开参考包。最后一级可用于估算与离线准备，保留估算标签；数据日期本身不阻止数学推导。这个顺序是待接入的解析策略，本次没有实现PC运行时热加载或改变Reader冻结协议。

当前模型ID为`published_reference.v1`，`readiness=reference_only`。没有提供的重力、阻力规则、插值切线、射手速度继承、发射点和瞄具归零保持null；已读到的合法0和false不得当作缺失。网站的距离/速度tooltip有显示精度，不能冒充原始FloatCurve全部精度。网站初速与曲线起点有差异的条目原样保留，见checks，不擅自缩放曲线。`Drag coefficient`只保留其原字段含义，不直接当成现实G1/G7弹道系数。

Reader接入当前武器时必须使用同一批`Equipment.active_slot → members.slots[slot].weapon`，核对类名和`source_identity`独立读取证据。类名候选`Weap…_C`由公开Blueprint派生并明确标记未做本轮Reader实机匹配；名称字典不是生命周期证据。切枪、附件或弹种变化选择新配置，缺失不沿用上一把；不同item_id的同名变体独立保留。

抓取和整理工具：

```powershell
# 已归档数据离线重建；不会请求网络
python tools/sync-weapon-reference.py

# 离线验证所有文件摘要、逐页重新解析、别名与数据包revision
python tools/sync-weapon-reference.py --verify

# 独立检查Reader或校准工具将来生成的包，不依赖公开网站目录
python tools/sync-weapon-reference.py --verify-pack 数据包路径.json

# 新快照使用新目录，逐URL缓存，单线程且间隔至少1.5秒
python tools/sync-weapon-reference.py --fetch --output data/weapons/新快照目录
```

工具只使用Python标准库，自带关键合同、单位和数值校验；schema也可交给任意Draft2020-12验证器。它只抓公开页面和固定提交中的JSON，不连接Reader/TTL、不修改用户设置，不更改VS项目与构建入口。后续射击角度的接口与推导见 [弹道设计](../../docs/BALLISTIC_AIM_DESIGN.md)。

revision由生产者分配，公开适配器目前使用自身语义内容的SHA256；不要求Reader用另一语言复制Python的数字序列化方式。原文件完整性另由manifest逐字节SHA核对。provenance使用通用source_ref/content_sha256/locator，可以指向网页、Reader分析产物或实际采样证据，不要求未来Reader伪造HTTP响应。
