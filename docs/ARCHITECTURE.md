# mousebox-aimbot 架构

2026-10-29按用户要求统一撤下正常页面顶部的大标题/简介，PageHeader组件及其调用和样式已移除，框架直接渲染叶页内容；设置内容保留导航与非视觉名称。日志操作并入原筛选工具栏，Reader连接卡不再重复来源标题和已确认/采集目标频率。保留业务卡片标题及真正错误/等待/空状态；没有改Reader订阅、协议或控制资格。

子页框架按用户2026-10-29最终决定只允许一级PageGroup：framework/pages负责纯元数据校验、路由/历史、单排PageScaffold和按需挂载，navigation/pageGroups不导入业务实现。READER连接/数据看板、鼠标调试、CS2 Recoil共用一级入口；摘要/源数据改由DataDashboard内容按钮与view查询切换，模式记忆限定当前工作区。旧嵌套链接兼容，递归/位置观察/下级样式撤下；不缓存隐藏源数据，离页停止取样。见[UI页面框架](UI_PAGE_FRAMEWORK.md)。

一级页签排序由共享指针交互与纯几何计算负责，PageOrder端口向shell的PageTabPreferences适配保存。位置使用全局UI偏好的稳定group/tab ID，与所选路由和工作区内容生命周期分离；拖动不重新挂载面板，松手一次提交，取消清理。Electron按组合并写入web-ui.json，旧ID过滤、新项追加；不经过Reader或原生控制链路。

PC2.8.2新增ESP两段本地耗时：domain/SourceReceipt随实际基础帧和SceneSubmission传递，render/frame_clock与frame_timing配对渲染入口及既有GPU完成通知；RuntimeState只传近2秒均值。渲染层不依赖游戏/应用或platform目录，新增时钟实现留在渲染模块。来源失效清空，最高伤害独立计算仍不阻塞当前帧。见[ESP帧计时](ESP_FRAME_TIMING.md)。

PC2.8.3新增按需源数据查看：DataSource的SourcePacket只暴露不可变诊断回调，CS2真实Envelope与PUBG SDK合并状态仍由各自适配器解释；通用SourceInspector后台整理一个最新工作项与结果，UI以reader.inspect在数据看板取样，关闭即停止。字段表来自冻结descriptor，原业务快照、控制资格和算法不依赖诊断JSON。通用JsonTree负责搜索/展开/虚拟滚动，不引用游戏。见[源数据查看](SOURCE_DATA_VIEWER.md)。PC2.8.4起Reader页面只组合features/reader的连接内容和features/data-dashboard的看板内容，外层查询参数决定子页；原/dashboard仅作兼容跳转，侧栏不再重复入口。未选中的子页卸载，源数据不会在连接页继续取样。

PC2.8.1增加READER每订阅的两秒三段计时。游戏适配器只翻译已有原始锚点，domain/reader_chain_metrics统一去重、隔离与有界聚合，application/diagnostics序列化汇总，shared/UI只消费三段均值、样本数和不确定度；接收端点在解码前，独立于GPU和控制。见[Reader链路计时](READER_CHAIN_TIMING.md)。

PC2.8.0将最高伤害瞄点独立到有界工作线程。普通扩展维持原串行组；最高伤害最近完成结果可短暂滞后，骨局部种子随当前姿态投影，不能选择旧计算帧或在绘制路径重新查询。原掩体、余伤和头伤的帧配对保持。见[最高伤害瞄点](CS2_BEST_DAMAGE.md)。

ESP输出由原生`window/surface_router`装配内嵌子窗口和独立`overlay_surface`，当前只在CS2声明并开放，PUBG保持原画布/显卡。`game_window`只读取前台目标进程身份与物理客户区。通用render的`composition_target`提供预乘透明DirectComposition交换链，D2D painter只区分透明背景，不知道游戏窗口或插件。区域、截图策略与引擎由Electron偏好唯一写入，仅对声明能力的游戏生效；UI只提交受限选项，私有桥由主进程附加已验证父窗口身份。前台轮询仅跟踪放置、不限制绘制帧率。见[ESP绘制区域](ESP_PRESENTATION.md)。

PC2.6.2增加CS2独立head-damage扩展与余伤下方的懒加载开关。首次表面求交放在hitboxes/intersection纯几何模块，目标输入/结果撤销/后台生命周期/标签分别由head_damage与presentation负责。应用端口只传目标状态和伤害/扣血范围，traceline适配器单次静态路径查询后调用公开目标伤害API，不复制内部公式。既有helmet仅补消费端解码，未扩Reader；规则/免疫/小数累计无实况时显式条件估算。见[伤害显示](CS2_REMAINING_DAMAGE.md)。

2026-10-25，PC2.5.0将CS2静态掩体正式接入按游戏扩展宿主。traceline适配器只依赖公开C API、固定DLL和相对路径缓存目录；通用平台层负责安全搜索范围的动态库加载与流式文件摘要，游戏扩展不认识DLL或文件。原生后台按需加载，结果绑定原快照/来源/地图代次后投影到通用装饰批次；缺图、坏包或ABI不符回退原色并报错，5秒重试。资源只打包转换缓存（现统一.cs2tl后缀），不纳入KV3、内部算法源码或额外VS工程。Reader继续1288地图契约，原Aimbot/RCS数学和资格不变。见[游戏扩展](GAME_EXTENSIONS.md)与[二进制验收](TRACELINE_ACCEPTANCE.md)。

PC2.3.0新增CS2独立Recoil页。`games/cs2/recoil`拆分参数、当前观测资格、纯差分量化与有状态OutputProgram；不依赖目标/骨点/相机几何。模块按RCS开关在原Aimbot与RCS之间绑定唯一输出所有者，通用activation描述盒子鼠标或本机键盘激活；原生实时线程在每次有限运动前重验。No Recoil仅空占位。Web使用独立懒加载面板、通用绑定控件与有界DTO，现有Aimbot历史版本数值保持。见[独立RCS](CS2_RCS.md)。

PC2.2.7统一ESP两类视觉缩放：纯render/esp_scale计算世界骨点/线宽和缓和的附加信息比例，各游戏投影只提交当前几何与局部尺度；D2D独立geometry组件批量生成渐变宽度与同比例描边。文字缓存通过变换复用，不随每帧尺度重新布局。具体曲线、源单位边界与验证见[ESP视觉尺寸](ESP_SCALING.md)。控制算法与Reader协议保持PC2.2.6行为。

PC2.2.6将CS2计算链拆为资格、参考、时序、几何、选人、反馈与运动模块，OutputProgram保持状态组合；提供v1–v4和16个固定v1的单变量组。PC2.8.25将实际运动移入公共control/movement，各游戏通过轻量AimMovement描述符接入独立选择，历史默认输出保持，单变量组固定原基线。PUBG只开放已有平滑策略，其轨迹数值计算亦从反馈协调器提取。新的人手模型可注册公共数值策略，无需改目标/后坐/Reader/设备层，见[鼠标移动策略](MOUSE_MOVEMENT_ALGORITHMS.md)。PC2.8.33界面按GameUiModule.aimGeneral登记共享模块，Aim常规装配版本/后坐、移动/速度、输入和时序/弹道；共用展示归features/aim/common，Aimbot只保留自身选人及触发参数。旧AimVersions/AimMovement页面插槽退役，Web仍用原useSettingBinding与稳定配置键。模块可由部分功能消费，实际范围见[Aim常规](AIM_GENERAL.md)。侧栏顶部移除项目名和“游戏”标签，游戏菜单与版本同一行，Select复用Base UI且保持原生画布遮挡管理。实现、模块边界及回退规则见[算法版本](AIMBOT_VERSIONS.md)与[计算流水线](CS2_CONTROL_PIPELINE.md)。下文PC2.2.4描述作为共享基础保留。

PC2.2.4，2026-10-24，CS2独立控制器与Aimbot页采用原生1:1公式默认、计数校准可选，配对最终Reader v1/0dd。公式是假设，不冒作硬件实测；真实跟随手感待用户验收。PUBG Consumer v3 与 CS2 Reader v1 独立接入。唯一开发目录为 `G:\mousebox-aimbot`；旧项目目录保留归档，不作为构建、运行或依赖来源。通信验证与持枪/游戏验收的范围分别记录在项目记忆中；真实乘车时Reader己方world速度缺失仍未修复。

PUBG 保留既有完整能力。CS2 的OutputProgram位于games/cs2/control，经同一OutputBinding使用全局设备所有者，不依赖PUBG Frame、控制器或串口实现；输入换算模式保存native/measured稳定ID。native使用明确未实测的1:1链路假设，measured必须有有效记录，不静默回退。原生导数已经包含符号/灵敏度/开镜倍率。CalibrationObservation只核验本地输入角、鼠标导数和可信采样，不借用目标/眼角/后坐条件；真正瞄准仍经过完整资格。普通第一人称输入分支支持目标锁定、时序/速度及P/U补偿；父变换/委托/实体世界角已解码展示，局部鼠标变换未核验时不输出。GTA5 仍未实现。每个已接入游戏一份独立配置，侧栏仅选择游戏。本轮未改PUBG算法或固件v7；CS2正式health/life-state允许本游戏明确死亡后保持停止，缺帧仍暂停同人，不套回PUBG。详见[CS2控制器](CS2_AIMBOT.md)。

READER 的连接子页组合各游戏来源连接与订阅面板，数据看板子页组合各游戏 DataPanel 与按需源数据查看；人物、武器、角度、FOV及运行遥测不混入连接内容。通用页面不内置 PUBG 兜底数据。旧根路径和 overview 启动参数仍导向 READER，旧 /dashboard 兼容跳转到其数据看板子页。

PC2.8.10 的 Triggerbot 由游戏描述符绑定独立面板，当前只有 CS2 声明 triggerbot 能力。games/cs2/trigger 将当前射击方向与资源 Hitbox 求交，Controller 管理新鲜度、前台资格、去重和节奏；可选 StaticCover 在后台加载地图。检测和提交直接查询已加载的静态场景，检查射击视点到首次命中交点的路径。输出装饰器保留原 Aimbot/RCS 移动程序。通用 OutputProgram/OutputDevice 只暴露有限按键效果，同一设备线程和有界队列发送，不开第二条串口，也不在 UI 或绘制线程计算。按下时长5–40ms直接送入既有v7.1有限动作；掩体的反应时间子项由 ReactionMonitor 后台骨点扫描及逐目标 ReactionTimers 实现；起读源时间经有效时钟映射到执行时刻，已耗延迟计入并将负等待归零。未接入游戏明确显示缺少核验支持。见[自动扳机](TRIGGERBOT.md)。

侧栏说明通过游戏描述符的 `navigationHelp` 数据声明，由通用 NavigationHelp/Tooltip 展示；Sidebar 不内嵌游戏原理。提示按钮位于导航链接右侧且与其并列，支持悬停/焦点/Escape、无障碍描述和原生表面遮挡owner清理。Triggerbot把原因/计数集中到共享页面上下文栏，开关与数值输入分列，页面不重复维护运行状态卡或原理卡。

PC2.8.9 的界面图标统一经 `components/icons/AppIcon` 和类型化语义键调用；页面、框架和游戏模块不直接导入图标库。`registry` 是唯一选图位置，按含义选择 MingCute 或保留 Lucide，不能仅为换风格替换更清楚的旧图形。MingCute 固定官方 `@mingcute/react` 3.0.2 的 Core Regular 单图路径，不使用全集导入、图标字体或运行时外网请求。默认正方形、继承 currentColor、装饰性 SVG 不抢焦点；独立表达含义时需明确无障碍名称，按钮/导航仍由控制自身提供名称。原 ⓘ 与 Tooltip 交互保持。提供方边界由架构检查约束。按用户长期偏好，分发包不带许可证/NOTICE等附带说明；打包排除规则与after-pack钩子同时检查ASAR和解包目录，原生资源准备不再复制许可证。

| 语义键 | 选图与原因 |
|---|---|
| connection | MingCute link-2：数据来源连接，不再使用电源插头 |
| logs | MingCute document-2：查看事件记录，区别于可交互终端 |
| mouseDevice | MingCute mouse：鼠标设备、原始报文和输入设置共用，不混为屏幕指针 |
| speedLearning | MingCute dashboard：速度仪表，区别于通用趋势统计 |
| dashboard | MingCute chart-bar：数据概况，区别于数据库存储管理 |
| triggerbot | MingCute cursor-3：点击触发，区别于扫描条码 |

其余语义保留清晰的 Lucide 图形，包括准星、眼睛、后坐方向、齿轮/调节、芯片、显示器、数据库存储、JSON括号、游戏手柄、ⓘ、搜索、流量箭头、展开/收起和窗口按钮。应用品牌 PNG/ICO、连接状态圆点、图表数据图元和 ESP 世界几何不属于这套界面图标，不随图标库切换。

CS2控制按SessionIdentity值对象识别Reader、时钟域、epoch、进程、完整本地controller/pawn和worldGroup。native_context在依赖该变量的分支用于本批配对；已核验的普通本地无委托/父变换source2输入角不依赖它，是正常切换的查询选择器；不能作为会话身份。simulation不承诺在相同world/context跨批单调，当前组独立核验与求值；采集序号/QPC高水位另行核验。临时缺组、乱序或调度中断保留目标，可信数据恢复后继续，不要求重新按侧键；响应核对优先使用已核验Pawn输入。真实松开计数跨回传间断保留，不把未知状态推定为松开；真正身份或设备/会话失效仍撤销资格。最后暂停原因与当前警告分离，诊断仅有界DTO。延迟与平滑后续设计见[优化方案](CS2_RECOIL_LATENCY_PLAN.md)。

本轮拆分与完整审视见 [2026-10-24 架构审视记录](ARCHITECTURE_REVIEW_2026-10-24.md)。通用控件通过 useSettingBinding 获取注册游戏的验证与参数，游戏面板逐一懒加载；DesktopSnapshots 负责发布，WorkspaceSwitchCoordinator 负责初始化/切换屏障。配置文档、迁移、事务分别拥有明确模块，D2DScenePainter独立管理图元和文字资源。

## 源码结构

```text
mousebox-aimbot/
├─ apps/
│  ├─ desktop/src/
│  │  ├─ main/application/             # 生命周期、命令、自动连接和表面协调
│  │  ├─ main/infrastructure/          # 原生进程、消息会话、流传输、偏好和迁移
│  │  ├─ main/ipc/                     # 来源校验与白名单
│  │  └─ preload/                      # 受限 typed API
│  ├─ ui/src/
│  │  ├─ shell/                        # 标题栏、导航、状态栏和游戏选择
│  │  ├─ pages/                        # 共用页面组合
│  │  ├─ framework/pages/              # 一级子页框架、路径与生命周期
│  │  ├─ navigation/                   # 一级页面组元数据，不导入业务视图
│  │  ├─ games/                        # 游戏界面注册及独立 PUBG / CS2 面板
│  │  ├─ features/                     # ESP、学习、模型、日志等交互
│  │  ├─ runtime/                      # 订阅生命周期、事件应用和状态
│  │  ├─ components/                   # shadcn / Base UI 组件
│  │  └─ styles/                       # 语义颜色、控件与布局
│  └─ native-host/
│     ├─ composition/                  # 正式模块装配
│     ├─ host/                         # 消息循环、命令和展示推送
│     ├─ bridge/                       # 有界消息、去重和应用用例
│     ├─ input/                        # C++ 原始输入、前台插件和二进制通道
│     ├─ window/                       # DirectX 子窗口
│     └─ cli/                          # controller / reader / replay
├─ packages/shared/src/                # Zod 契约、DTO、游戏与工作区身份
├─ src/
│  ├─ domain/                          # 数学、坐标单位、采样来源和通用 Scene
│  ├─ control/                         # 运动、反馈、学习、速度比例和弹道数学
│  ├─ application/
│  │  ├─ ports/                        # 游戏模块、数据源、设备端口和展示提交
│  │  ├─ output/                       # 可独立运行的输出调度与设备观测
│  │  ├─ workspaces/                   # 单游戏配置索引、迁移、隔离和保存事务
│  │  ├─ assets/                       # 公共模型、采样和报文资产路径
│  │  ├─ storage/                      # JSON、校验和原子保存
│  │  ├─ diagnostics/                  # 全局设备状态、通用日志和格式化
│  │  └─ tasks/                        # 有界后台队列
│  ├─ games/
│  │  ├─ registry.*                    # 能力与可用性目录
│  │  └─ pubg/
│  │     ├─ model/                     # PUBG Frame、身份、相机及骨点
│  │     ├─ pipeline/                  # 目标、资格、提前量和游戏映射
│  │     ├─ runtime/                   # 原 PUBG Backend 的真实实现
│  │     ├─ settings/                  # PUBG 参数、校准和应用
│  │     ├─ diagnostics/               # PUBG 状态与算法说明
│  │     └─ presentation/              # PUBG 投影、ESP 及预选装配
│  ├─ adapters/
│  │  ├─ sources/pubg_reader/          # 冻结 Reader SDK 的唯一解释边界
│  │  ├─ sources/cs2_reader/           # CS2 信封、快照和生命周期
│  │  └─ outputs/mousebox/             # 串口会话、能力协商和反馈
│  ├─ protocol/                        # 盒子线协议、Protobuf 基础读取
│  ├─ platform/windows/                # 通用字节传输、时间和输入
│  └─ render/                          # 纯图元、投影端口、GPU 和完成统计
├─ data/games/pubg/weapons/             # 游戏资料
├─ protocol/sources/pubg_reader/        # 冻结协议，固定摘要保持
├─ mousebox-firmware/                  # 独立多硬件固件，本轮保持 v7
├─ projects/                           # core / native / local tests
├─ tests/                              # 本地回归及夹具
├─ tools/                              # 构建、架构核验及 VS 目录同步
├─ mousebox-aimbot.sln                  # 正式两个 C++ 项目
└─ mousebox-aimbot.local.sln            # 本地另加测试项目
```

## 职责和依赖

领域数据不依赖具体游戏、平台和应用；控制数学只消费通用领域与协议。平台串口只传字节，不理解盒子协议。Source 与 Output 适配器不能互相调用。冻结 Reader SDK 只在来源适配器使用。

PUBG Reader 在线消费使用正式 Consumer v3 typed State / Receiver、业务能力协商和 19 / 20 号载荷。完整 SDK 保留原字节，只编译其显式声明的 17 个 client CPP，不编译 11 个 provider CPP。PUBG 适配层解释不透明实体 / controller / weapon / native-context 身份、独立来源时间及语义骨架；不得向 Debug 原生地址或 serial 造值。既有 v2 丰富记录仅经显式文件回放入口转换为 Debug 验证，在线 Session 拒绝 v2。冻结值、来源规则及验证范围见 [Consumer v3 接入](READER_CONSUMER_V3.md)。

游戏解释坐标轴、单位、相机基向量、骨连接、身份及有效期。通用 Scene 带空间身份、代次和来源证据，不内置 Unreal 单位或 PUBG 骨点编号。不同游戏模型不能相互导入。

`GameModule` 在真实路径中负责数据源、停止、资格、工作区重置及场景提交；`GameConfiguration` 将具体游戏参数规则与通用存储分离；`OutputDevice` 用于真实有限运动/停止。原 Backend 的命名空间为兼容调用暂保 `mousebox::application`，但实现、模型、规则位于 `games/pubg`，不能将其当成其他游戏的运行时。

宿主通过 GameApplication 将配置、状态、原理、模型与绘制完成事件交给当前游戏模块。PUBG 的游戏状态由既有适配层生成。全局的串口、盒子与绘制设备状态由设备观测直接生成，再由宿主合入所选游戏的状态。CS2 的状态请求只读取设备观测和自身诊断。NativeHost 目前仍通过 Backend 兼容层装配工作区、设备租约与 PUBG 校准。游戏模块通过 OutputBinding 装配其输出程序，配置注册位于 application/settings/game_configurations.cpp。

DirectXRenderer 负责设备、交换链、场景邮箱和完成统计。D2DScenePainter 负责图元及文字资源。调用方提供字体字节并保证它们活到 DirectXRenderer 销毁，以便设备恢复时重新读取。宿主仍从自身资源加载生产字体。具体游戏提供不可变的投影请求，唯一绘制线程拥有该投影的可变状态。场景提交关联的 Reader 样本保存在应用层，宿主通过随帧回调接收 GPU 完成时刻并更新对应订阅指标。完成通知仍有界，按实际提交帧关联。

需要随场景缩放的ESP圆环以通用 `Circle{center, radius}` 传给GPU，具体游戏投影层负责世界尺寸、相机深度和实时FOV换算。PUBG头圈直径由已核验头颈骨段估算，不代表碰撞体；GPU按图元半径批绘制，不能重新覆盖为统一像素大小。关节提示点仍使用固定界面尺寸。

## 进程、消息和工作区

Electron / React 保持 pnpm workspace、shadcn base-nova / Base UI、语义 CSS、主题和中英文。VS 正式只保留 core 与 native，本地增加 tests。前台输入插件由已有 native 项目构建，完整原始输入仍在 C++ 线程和本机二进制通道，JS 不逐报文调度。

PC2.6.1的可调整侧栏由`features/navigation`负责交互与CSS预览，shell/Sidebar只组合拖动边。PC2.8.7加入DIMO式临界收起：展开宽度150–240（初始175；2026-10-31按用户最新要求调整），收起视觉宽度56，超出展开最小值向左拖150才折叠，收起后向右拖150才展开；每次切换重新锚定，连续拖动不跳变。`sidebarDrag`只计算状态，`sidebarLayout`负责CSS变量和根节点模式，hook负责指针/键盘、独立遮挡owner和松手提交；每帧预览不写React/IPC，阈值内反向移动不抖动。Electron全局分别保存sidebarWidth和sidebarCollapsed，旧配置默认展开，partial更新不注入默认值；不混入游戏配置或原生算法。取消还原原宽度和模式，失焦/丢捕获释放所有资源；键盘左右可在边界收起/展开，Home/End仍设置展开范围，双击/Enter恢复展开175。收起时隐藏导航文本但保留ARIA/悬停标题和游戏图标菜单；不能重新加入被移除的品牌图标。响应式样式不得覆盖宽度。隐藏验证只分发隔离DOM事件，不注入OS输入。软件图标以data/branding/application-icon.png为唯一美术来源，格式转换与打包缓存分别追踪输入和产物，不再生成旧M标识。

Web 只传用户命令和有界展示 DTO；不逐包传 Reader、Raw Input 或 GPU 像素。会话、长度封套、请求 ID、去重、队列上限、来源校验、租约及父进程死亡回收保持。关闭窗口撤销资格、停止控制和采集、恢复光标并退出。

业务消息带 `{workspaceId, gameId, epoch}`。原生校验命令，桌面过滤过期回复/事件，UI 再核对异步继续操作和订阅。切换先撤销资格，排空采集/文件任务并取消拟合，再清来源、场景和校准状态，提交新配置。失败保持输出停止，只读刷新真实身份，由用户明确重新载入恢复。

全局偏好、`application`（DPI/显卡）和 `devices`（串口、盒子、GPU 白名单）不受游戏切换影响。`devices` 不含 Reader、人物或目标。`state/values/espValues/learning/mouseReports/algorithm` 按工作区校验；日志保留发生时的身份。目录先于新数据发布；最小化缓存丢弃旧代次；文件选择后再次核对上下文；旧表面清理不能隐藏新表面。

## 数据

数据根保持 `%LOCALAPPDATA%\Mousebox`。C++ 唯一写业务配置与资产，Electron 唯一写主题、语言和窗口偏好。首次保留旧文件原字节，核验备份后建立 PUBG 默认工作区，最后提交索引。模型/采样文件不移动或删除；新资产进入 `assets`，模型启用关系按工作区保存。

多文件保存使用事务日志并支持启动回滚；损坏的新配置明确失败，不静默变为默认。旧版没有落盘的内存状态无法凭空恢复。详情见 [工作区与用户数据](WORKSPACES.md)。

新增可默认的游戏参数通过 `GameConfiguration.permitsAbsentValue` 显式声明旧文件缺失兼容，通用工作区层不硬编码游戏字段名；显式非法值及其他必需参数仍严格校验。PUBG的 `livingLocalSkew` 缺失采用100ms，八项新增枪械分类部位缺失采用-1继承原默认。跨流补偿上限随 `MotionConfiguration` 进入控制与诊断，独立于整帧配对和同一Living目标检查，见 [时间差设置](LIVING_LOCAL_TIMING.md)。

PUBG纯部位与类别策略位于model，pipeline集中解析当前武器，实际控制、ESP预选和说明共用，Web只展示原生解析结果。分类数据由本地公开资料离线生成，不向Reader新增字段。相机平移对齐共用于选人和弹道，骑乘总速度不得再加一次载具速度；上游world缺失保持明确拒绝。详见 [枪械类别与乘车校验](PUBG_WEAPON_AIM.md)。

## 2026-11-16 解耦边界

| 模块 | 负责的接口 | 单独消费所需依赖 |
| --- | --- | --- |
| OutputLoop | 以单次 tick 调度桌面测试、有限位移和点击。提交前核验当前资格、反馈及程序版本 | C++20、共用盒子协议头、实现 OutputLink 的设备、时钟与按键回调 |
| SerialSession | 维持唯一串口连接与工作线程，打开时协商能力，退出时停止并排空发送队列 | Windows 串口适配和盒子协议。通过注入的执行回调连接调度器 |
| Protobuf Reader | 借用字节视图读取字段号、变长整数、定宽整数和字节块 | C++ 标准库。CS2 解包及源数据查看分别管理字段、深度和大小校验 |
| BundledFont | 从调用方字节建立私有 DirectWrite 字体集合 | Windows DirectWrite、调用方提供的字体字节 |
| FramedJsonPipe 与 FrameCodec | 对 Node 流读写长度前缀 JSON，处理分片、结束和写队列上限 | Node 流和 Buffer，TypeScript 源码可独立编译 |
| NativeSession | 校验封套、关联请求与超时、工作区切换后取消旧请求 | 应用消息契约和注入的发送函数 |
| runtime/events | 应用已校验事件、更新状态与语言投影 | 界面状态和展示函数。订阅、重试与首帧竞争由 connection 处理 |

输出调度使用既有 OutputProgram 承载 CS2 的计算，PUBG 的跟随与实测校准通过游戏适配进入同一设备工作线程。设备状态和物理按键不从游戏帧推断。CS2 Reader 信封保管协议版本、握手和消息封装，快照解码保管骨骼、武器与覆盖范围等模型规则，并保留源数据查看所需原始字节。固件仍按核心、协议、平台和目标装配，验证入口独立。

## 检查与扩展

```text
pnpm check:architecture
pnpm test:architecture
pnpm typecheck
pnpm lint
pnpm test
tools/build.ps1 -Configuration Release -IncludeLocalTools
tools/build.ps1 -Configuration Debug -IncludeLocalTools
out/x64/<配置>/mousebox-tests.exe --suite all
python tools/sync-vs-filters.py
pnpm dist:win
```

架构检查使用TypeScript AST读取多行/type/懒导入与re-export，约束公共控件、游戏模块、runtime、desktop domain及核心业务编排；C++继续约束分层和冻结SDK，存储不再有访问游戏目录的特殊例外。core工程已移除宿主include目录，另有项目配置守护。静态检查不替代编译、链接和运行资格验证。`.filters` 是源码职责的 VS 浏览视图，不增加工程。第三方、冻结 SDK、固件发布和历史证据不参与批量格式化。

可在正常启动时显式指定 `--timing-report=<绝对JSON路径>`，由当前Electron拥有的NativeClient经既有两项read接口按1Hz采20秒，最多20份，只保存白名单时间、状态和计数。游戏诊断层负责时间DTO，桌面层负责有界调度、隐私过滤与原子文件更新；缺省不开启。`complete=false`是部分记录，最终为true。它不修改会话资格、连接或时间策略，不开启额外服务；报告清楚标识启动后的新现场，不能代替旧进程的告警证据。`mapped_age_ms_unqualified`仅为便于核验的未获资格映射年龄，不能作为控制输入。重新执行需要再次显式启动，不自动持续运行。

新增游戏见 [接入流程](ADDING_A_GAME.md)、[界面接入](ADDING_A_GAME_UI.md)。旧路径及 PC1.2.8 验证范围保留在 [历史架构](history/ARCHITECTURE-PC1.2.8.md)。

## CS2 v1

只监听本机的 Reader v1 独立 wire codec / client 位于 adapters/sources/cs2_reader；冻结三份 proto 和 descriptor 在 protocol/sources/cs2_reader，由独立构建检查守护。独立配置、模型、模块、诊断和矩阵投影在 games/cs2。公共 domain/control/render 不包含 CS2 类型，源适配器只允许引入本游戏 model。TCP 帧字节在 platform/windows/framed_socket 复用，PUBG 的探测盖戳保持在 PUBG wrapper。

完整快照替换、失效/断线清空、实例/epoch/完整 handle 及单调序号隔离。全批共用实际采样区间；以 ClockQuery 往返建立 QPC 映射，不给 Local/人物伪造独立时刻。近一秒流量、接收帧率和 GPU 完成延迟只统计实际事件；无 GPU 完成时显示等待绘制。

使用 world-to-clip 投影本批已核验骨点，语义来自独立BoneRole、连线来自Skeleton.links，不按native index猜测；未知模型保持未知，缺点不补旧帧。HEAD为骨原点，不推断命中盒或头圈；不用 fov_override 当最终 FOV。单位比例默认未知；显式配置后才启用米制角色距离。控制器距离筛选采用明确原生单位，与未知米制比例解耦。输入原生导数已核验，但设备计数比值必须独立实测并由用户采用，不能默认为1。详见 [CS2 接入](CS2_READER_INTEGRATION.md)。

CS2的纯数学参考recoil.hpp为固定hash上游交付，由games/cs2/evaluation唯一包装，不引入传输或设备调用；adapters/sources只解释wire与来源状态。诊断拥有有界DTO，UI展示本帧原始量与有条件派生量。条件basis与AimPunch原生context不齐不混合，不借网络原始角回退。来源1–7由Reader预先求值，消费者不重复历史插值、父变换或委托；委托来源不替换后坐所有者，缺失不清人物/骨架。纯求值层继续无副作用；独立OutputProgram负责可取消的控制意图、推测反馈账本与有限步输出。game.*命令在TS/桌面/原生均纳入工作区代次核验。

算法分组可显式声明aimbot/mouse范围，摘要由各游戏提供；旧PUBG分组兼容原范围规则，不将其S曲线说明套到CS2。CS2设置、控制及设备校准均有中英文；页面仅装配对应懒加载面板。


PC2.8.10的辅助说明由shared/HelpTooltip与控件help属性声明，游戏和页面只提供内容；静态说明不占表单行，真实状态/错误/数据继续显示。通用Tooltip负责悬停、焦点、Escape、定位与原生表面owner，不在页面建立重复弹层。字段label和帮助按钮并列，避免帮助点击触发开关。
