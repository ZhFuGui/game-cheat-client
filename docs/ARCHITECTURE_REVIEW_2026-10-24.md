# 架构审视与解耦记录：2026-10-24

本轮面向 mousebox-aimbot 全项目审视，实施范围是原生配置存储、渲染、Electron 业务编排及 Web 游戏模块。开发根为 G:\mousebox-aimbot。结构重构阶段保留固件、冻结 Reader SDK/proto、游戏控制与弹道数学，并先完成独立回归。随后 Reader 窗口按其人类指令正式升级 CS2 骨架协议，本窗口将这一路适配单独核验后合入 PC2.1.3，详见 CS2 接入文档。PUBG 与设备规则保持。

## 审视结论

现有“原生负责实时工作、Web负责命令和展示、游戏模块解释数据”的大方向成立。主要问题集中在组合边界：通用表单知道具体游戏、同一个对象同时承担迁移/事务/查找、桌面路由持有平台与切换流程、GPU调度同时拥有全部画刷和文字缓存。原有静态检查还依赖导入正则，可能漏过多行或类型导入。

本轮按职责与资源所有权拆分，没有按文件长度平均切块，没有新增 VS 工程，也不引入服务容器或空转接口。代码行数只用来辅助定位，不作为架构优劣的单独指标。

| 区域 | 原问题 | 本轮结果 |
| --- | --- | --- |
| 公共设置控件 | 根据 gameId===cs2 分支校验，否则默认使用 PUBG；大量类型断言 | useSettingBinding 负责参数归属、字段验证和类型化提交；输入、选项、开关均统一拦截非法提交，控件不再判断游戏 |
| 游戏 UI 装配 | 注册目录静态拉入全部页面；CS2入口混合实现，ESP默认回退PUBG | 轻量模块描述符、逐面板懒加载；游戏显式声明自己的ESP；未知游戏无PUBG兜底 |
| 配置存储 | 索引查找、历史导入、SHA备份、事务恢复及游戏目录混在 Store | 文档校验、索引迁移、双文档事务独立；Store保留索引和协调 |
| 跨游戏依赖 | 通用存储直接引入 games/registry | 具体游戏目录投影归配置组合入口，删除存储层例外 |
| 桌面命令路由 | 路由读取INI、装饰快照、管理切换、操作平台类型 | 路由仅组合命令；快照服务、切换协调器、平台端口分工 |
| 并发资格 | bootstrap 与切换可交错，再次激活或启动旧来源 | bootstrap/switch共用专门串行屏障，普通命令不全部串行化；切换失败后bootstrap只读 |
| 消息作用域 | native与renderer共用的作用域规则放在native基础设施目录 | WorkspaceMessageScope归桌面domain，双方共用同一规则 |
| IPC白名单 | in 操作符会把原型成员视为已知命令/事件 | 自身属性检查，未知及原型名字明确拒绝 |
| DirectX | 交换链/调度与D2D绘制、字体、画刷缓存混合 | D2DScenePainter独立拥有绘制资源，Renderer管理设备、邮箱、呈现和完成统计 |
| 自动约束 | 正则可能漏多行/懒加载/type导入，公共控件可反向依赖游戏 | TypeScript AST解析与新增层级规则，正反例测试守护 |

## 当前职责结构

~~~text
apps/
├─ desktop/src/main/
│  ├─ domain/
│  │  ├─ ports.ts                         # 原生、偏好、文件命令等窄端口
│  │  └─ workspaceMessageScope.ts          # 消息上下文与代次规则
│  ├─ application/
│  │  ├─ commandRouter.ts                 # 命令组合
│  │  ├─ desktopSnapshots.ts              # 桌面元信息及目录/状态发布
│  │  ├─ workspaceSwitchCoordinator.ts    # 切换/初始化屏障与失败恢复
│  │  └─ container.ts                    # Electron/平台组合根
│  ├─ infrastructure/                    # 管道、偏好、旧资料读取
│  ├─ ipc/                               # 输入与来源白名单
│  └─ window/                            # 窗口、原生表面与生命周期
└─ ui/src/
   ├─ components/shared/SettingField.tsx  # 表单交互与呈现
   ├─ hooks/useSettingBinding.ts          # 参数所有权、验证、提交
   ├─ games/
   │  ├─ contracts.ts                     # 显式模块/字段/面板契约
   │  ├─ registry.ts                      # 轻量注册与查找
   │  ├─ pubg/                            # 专用面板，包含PUBG ESP
   │  └─ cs2/                             # 专用数据/原理/ESP与纯装配入口
   ├─ pages/                              # 组合模块；局部Suspense
   └─ runtime/                            # 唯一事件所有者及状态投影
src/
├─ application/
│  ├─ settings/
│  │  ├─ configuration_documents.*        # 全局/游戏文档分拆与严格验证
│  │  └─ game_configurations.*            # 配置适配与可用游戏组合
│  └─ workspaces/
│     ├─ workspace_store.*                # 索引、活动项、资产引用和协调
│     ├─ workspace_storage.*              # 路径与多文档提交/恢复
│     └─ workspace_migration.*            # 旧来源、唯一配置、SHA备份
└─ render/
   ├─ directx_renderer.*                  # 线程/邮箱、设备、交换链、完成事件
   └─ d2d_scene_painter.*                 # 画刷、文字、几何与HUD绘制缓存
~~~

正式 VS 继续只有 core/native，本地另有 tests。core项目不再包含native-host搜索路径；核心不能靠宿主头文件意外编译成功。

## 行为与所有权约束

配置迁移保持“校验与备份在前，索引原子提交在后”。prepared事务必须先校验路径及两份旧文档，再按原顺序恢复游戏文档和全局文档，committed日志只清理。复核发现旧恢复路径遗漏全局文档校验，已补齐；损坏日志保留，两个有效现文件均不覆盖。未知扩展元数据、无变化不重写、原文件/模型/采样路径保持。磁盘 workspace-index.json、workspaces/<id>、wire字段及原ID不改名。

切换一旦请求便阻止第二次切换和旧业务写入。先开始的bootstrap可完成其整段工作，随后切换撤销旧资格；切换期间到达的bootstrap等待新目录。失败后bootstrap只能读取真实状态，不激活、不自动连接。激活回执超时可能已经到达原生，因此失败处理先再次撤销，再刷新身份。窗口生命周期仅在实际激活成功后记状态。

D2DScenePainter由唯一绘制线程创建、调用和销毁，持有自身COM引用，不拥有交换链、消息队列或时钟线程。设备恢复销毁旧绘制资源，缩放/尺寸变化重建网格；缓存不跨设备复用。HUD只接收GPU完成帧率读数，不反向访问Reader/游戏/设备。非ESP路径不额外扫描HUD计数桶。统计读数时点由HUD内部移到该次绘制之前，极端跨过毫秒滚动窗口边界时，显示数字可能短暂不同；计数来源和窗口定义不变。

公共页面局部等待懒面板，不把整个外壳或DirectX表面包进全页加载边界。配置、采集与表面的旧epoch清理规则保持，JS不接收原始鼠标包、Reader全帧或GPU像素。

## 命名约定

- C++文件继续使用 snake_case，类型用 PascalCase，方法/局部变量用项目既有 camelCase；不改冻结或第三方命名。
- React组件/页面用 PascalCase，hooks以use开头，模块查找/动作函数表达明确动词，例如getGameUiModule、getGameSettingValidator。
- 类与文件按实际职责命名：DesktopSnapshots不是泛用“服务”，WorkspaceSwitchCoordinator只协调切换，WorkspaceMessageScope只处理消息作用域。
- 画刷按用途命名entityBrush/targetBrush/textBrush，缓存按内容命名labelLayouts/gridGeometry，避免主题变化后颜色名与实际含义不符。
- application/global、game configuration、Reader connection、runtime lease、workspace epoch分别描述不同范围；不将一次网络重连叫作游戏切换。
- 公开协议和持久化标识保持稳定。内部仍保留WorkspaceStore/WorkspaceContext等兼容名称，并明确其对应“每游戏唯一配置/消息会话”，不恢复多配置产品入口。

## 检查与证据

已增加AST边界正反例：多行、type、懒导入、require、注释示例；禁止公共控件依赖具体游戏/页面、游戏互导、runtime依赖游戏UI、桌面domain依赖平台和业务实现。只在真实组合根保留具体实现依赖。配置存储访问游戏目录的旧例外已删除。

验证包括：Debug/Release构建和原生全套、配置迁移/失败回滚107项、桌面受控Promise并发、IPC原型成员拒绝、两游戏表单及面板边界、C++/TS契约互验、隐藏桌面导航与故障清理。最终数量和发布状态以PROJECT_MEMORY本次条目为准。

渲染前后用同一机器、同一WARP和字体、四个固定合成场景，覆盖普通预览、ESP、文字、不同头圈半径、HUD、中英文及1/1.25缩放；400×300 BGRA输出逐字节一致。窗口全程隐藏，不抓取用户桌面，不注入输入，不把这些结果当成真实独显/游戏性能验收。

最终PC2.1.3：Debug/Release原生all、42个Web文件295测试、11项架构守护、469源码边界、typecheck/lint及打包隐藏Electron检查通过；正式包131文件，43项UI资产、ASAR main及原生逐SHA核验。原生SHA f604b4cbe949f1c735e8eb8528e4d3a174bd2a3dfdf0f20fe7f82896c7702ff5。76份用户文件换版前后逐字节保留，旧2.1.1归档，当前2.1.3最小化运行。Web日志仍有部分React测试act提示、Vite基础入口体积提示和builder上游DEP0190提示，未隐藏这些告警。

证据：G:/mousebox-analysis/architecture-review-20261024为结构阶段，含源码基线/职责指标、编译测试、paint-before/paint-after-final/paint-comparison-final，以及隔离桌面与打包结果；G:/mousebox-analysis/cs2-bones-20261024为最终配套阶段，含107项迁移/事务检查、协议/只读验证、全量回归、发布备份与启动。52份控制、PUBG模型/管线源码与最初基线SHA一致。

## 尚未消除的架构债务

| 优先级 | 项目 | 后续边界 |
| --- | --- | --- |
| 高 | PUBG Backend仍兼管唯一全局设备所有者 | 逐步提取DeviceController，再让PUBG/CS2通过OutputBinding接入；需独立验证串口停止、热插拔和输出资格，不能只改名 |
| 中 | RuntimeService::Impl仍汇集学习、模型和原始采集状态 | 按任务/资产会话提取状态所有者，保留停止和排空屏障；不让后台任务反向持有整个宿主 |
| 中 | shared的旧Settings聚合和RuntimeState扩展字段兼容层 | 逐游戏收窄DTO，并采用显式迁移；不能把PUBG字段强加给新游戏或丢弃现有null/0语义 |
| 中 | CS2 codec/client仍较集中 | 在新协议单独冻结后再拆传输、时钟和接收状态机，以正式夹具证明时序/存在性；本轮仅单独完成正式骨架协议适配，未同时拆状态机 |
| 中 | fileUseCases、diagnostics、rendererEvents和部分startup仍含窗口适配 | 后续归入专用窗口/基础设施适配器；当前commandRouter通过窄端口使用，不将同目录视为全都纯业务 |
| 低 | 通用绘制等待文案仍有Local/Living来源术语 | 后续让SceneProjection提供等待文案，不把PUBG专有概念继续扩散到painter |
| 低 | 页面事件描述与旧文档中还有兼容workspace术语 | 保留wire/数据ID，逐步清理显示用词；不批量重命名用户文件 |
| 低 | 单个Vite基础入口仍有体积提示 | 已实现游戏面板真正分块，继续按实际入口分析依赖；不通过提高阈值掩盖告警 |

这些是明确保留的后续工作。本轮没有声称整体完全解耦，也没有为了消除所有长文件把控制算法或输入热路径机械拆散。

