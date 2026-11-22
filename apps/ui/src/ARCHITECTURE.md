# Web 界面模块边界

PC2.2.6：侧栏顶部仅游戏菜单和版本号，同一行；移除项目名与可见“游戏”标签。GameSelector沿用既有切换屏障/失效恢复，Select的menu变体提供文字箭头触发、圆角菜单与当前勾选，表单field变体保持原行为；两者共用Base UI与SurfaceStore遮挡所有权。

Aimbot算法模块提供4个CS2版本和16个单变量组。标准版本与对照组只写同一个aimVersion，选择对照时固定base=v1；字段、唯一改变、生效条件与当前相关功能未启用提示来自共享目录。UI不参与高频计算，PUBG不显示未实现的CS2组。实际模块见docs/CS2_CONTROL_PIPELINE.md。

PC 2.1.4，2026-10-24，原生状态只读接入已配套换版。产品名称统一为 mousebox-aimbot。界面保留 Electron / React、DIMO 侧栏布局、语义主题和中英文，侧栏直接选择游戏，每款游戏一份配置。PUBG 保留完整能力，CS2 提供独立只读数据与矩阵 ESP，GTA5 unavailable 不显示为可选项。

## 依赖与目录

```text
shell/                         应用外壳、全局导航、标题栏、底部连接与流量
workspaces/                    当前游戏路由与能力边界
pages/                         共享页面组合
games/
  contracts.ts                 游戏 UI 模块、字段验证与设置面板接口
  registry.ts                  编译时注册轻量描述符与字段验证入口
  GameSelector.tsx             侧栏游戏选择
  pubg/                        PUBG 数据、ESP、跟随、灵敏度、校准与原理
  cs2/                         CS2 数据、矩阵 ESP、数据设置与原理
hooks/useSettingBinding.ts     参数所有权、注册字段验证与类型化写入
features/
  settings/                    全局常规、固件、输入设备、绘制和数据范围
  esp/                         共用 ESP 状态、面板可见性与 Insert 行为
  aimbot/                      共用运行状态、能力比例速度及算法事实展示
  learning/                    原始速度采样、模型与生命周期
  mouse-debug/                 原始鼠标特征采集事务及完成结果
  page-context/                页面事实、故障优先、去重及轮播
  logs/                        有界虚拟列表
runtime/
  state.ts                     全局 applicationValues、当前工作区及兼容展示聚合
  workspaceState.ts            换工作区时保留全局事实的纯投影
  connection.ts                bootstrap 与所有事件订阅的唯一所有者
  displayProjection.ts         原始文案缓存与当前语言投影
lib/desktopApi.ts              契约、命令作用域、上下文校验
```

依赖方向：外壳/页面 → 工作区与领域组件 → 显式动作 hook / 状态选择器 → 受限 desktopApi / shared。shared 不导入 UI 或 Electron。页面不判断 PUBG 枪械/骨点，也不拼接任意游戏参数；专属设置由注册的 GameUiModule 提供，不通过万能 JSON 表单渲染。

通用表单只渲染输入、选项和开关，通过 useSettingBinding 读取参数并提交命令。DPI / 显卡参数使用 applicationSettingsSchema；ESP 使用既有共用字段契约；游戏参数由 GameUiModule.settingValidators 声明。公共组件没有游戏 ID 分支，未知或无活动游戏不默认采用 PUBG 验证。SettingTarget 区分 ESP 与业务参数键，写入无需绕过类型检查。

Reader 帧、原始鼠标报告、控制循环与 GPU 画面不跨 Web 桥。DirectX 仍由原生独立线程按数据到达更新，Web 只提交表面矩形、遮挡状态和用户命令。

## 导航与配置范围

侧栏顶部只有正式游戏名下拉框。原生为每个 available 游戏自动保留一份配置；新建、重命名、管理页面及组件已删除，旧 /workspaces 入口仅重定向 READER。同一时间运行一个游戏会话。

- 当前游戏：READER、数据看板、ESP、Aimbot、游戏设置。PUBG 的灵敏度、校准、算法来源和鼠标移动原理留在专属模块。
- 全局工具：鼠标移动调试、运行日志、设置。调试与日志继续相邻放在侧栏底部。
- 全局设置依次为：常规、固件、输入设备、显卡与绘制、数据管理。
- DPI 与显卡选项全局共享。Reader 端点、灵敏度、ESP、控制/弹道参数、校准以及模型启用关系属于工作区。
- 原始采样/模型是公共文件资产，保留原始内容和来源；不会由于切换游戏自动启用。
- F8 桌面测试与端口刷新在全局输入设备页，游戏校准保留在对应游戏设置页。

数据根目录继续为当前 Windows 用户的 %LOCALAPPDATA%\\Mousebox。Electron 唯一写入界面偏好，C++ 唯一写入业务配置与工作区索引。shared 的 applicationSettingsSchema 和 pubgSettingsSchema 分开定义，原 settingsSchema/values 是兼容现有命令的聚合投影，不表示文件仍由一个全局游戏配置管理。

## 跨工作区一致性

shared WorkspaceCatalog 声明稳定 workspaceId / gameId 与原生生成的不透明 epoch。游戏能力只有已知 ID：esp、aim、input-calibration、algorithm-details。能力表示实现存在，输出资格仍由原生根据实时数据和设备判定。

- bootstrap 可以缺少 workspace，仅用于旧的隔离夹具；生产界面在无目录时显示等待，不虚构 PUBG 连接。
- desktopApi 的工作区命令携带上下文，主进程和原生继续复核。全局停止、隐藏表面、连接设备不依赖活动游戏。
- 正式工作区的 values/state/espValues/learning/mouseReports/algorithm 事件必须匹配 workspaceId、gameId 和 epoch；旧代次和无来源事件丢弃。全局 application 事件和 bootstrap.application 是 DPI / 显卡设置的权威来源；兼容 values 中的全局值不能覆盖更新的 application。严格白名单 devices 快照独立管理串口/盒子、能力和 GPU 事实，active 为 null 时仍可更新，不接受 Reader、目标或输出资格字段。速度能力与全局日志独立保留。
- workspace 事件先清除旧游戏状态、算法和翻译源，保留设备/GPU事实；新参数和状态到齐后解除切换展示。
- 外壳的 Outlet 以 workspaceId:epoch 为 key，重建旧页面表单、原生画布和采集生命周期。原生表面 hook 也捕获上下文，旧 cleanup 不能隐藏新画布。
- 用户动作 hook 捕获开始时的上下文。文件选择、模型读取或算法读取晚完成时不能回写新游戏；旧表单闭包不能提交到新工作区。晚到的工作区命令回复也不能撤回更新的切换事件。
- 页面上下文栏保留故障优先、恢复撤销和重复去重，轮播身份包含 epoch，不带走旧游戏消息。日志保留可选来源并支持范围筛选。
- 切换失败保持输出撤销。用户从侧栏重新选择游戏恢复；同 ID 重载也应产生新 epoch。不会把恢复按钮或保留配置标记当作输出已经就绪。

上述是跨工作区隔离；不会修改同一 PUBG 会话内既有侧键、目标缺帧或停止语义。

## 性能与资源所有权

状态消费者使用字段选择器，多字段使用 useShallow；高频遥测不能重绘正在编辑的表单。没有逐 Reader 帧、逐鼠标包或 GPU 像素订阅。日志保留 4000 项缓存与虚拟列表。

ESP 和采集区域只存在于对应路由。弹窗/选择菜单保留原生遮挡计数，区域确认后才能开始采集，失焦/卸载仍撤销采集并恢复光标范围。游戏注册表只同步加载描述符和字段验证；各游戏的连接、数据、ESP、Aimbot 和游戏设置面板分别使用 React.lazy，页面的局部 Suspense 保留外壳与原生画布。进入 READER 不会求值游戏输入、校准或算法面板。游戏模块必须显式声明 EspSettings，公共 ESP 页没有 PUBG 回退；AimSettings 只在有瞄准实现的模块提供。

## 回归入口与范围

```text
pnpm --filter @mousebox/shared typecheck
pnpm --filter @mousebox/ui typecheck
pnpm --filter @mousebox/ui lint
pnpm exec vitest run apps/ui/src/test
pnpm --filter @mousebox/ui build
pnpm check:architecture
```

workspaces.test.tsx 覆盖目录和作用域校验、无目录空状态、游戏能力、切换清理和全局保留、旧事件/无来源事件、旧表单与文件选择、游戏重选恢复，以及全局 DPI 与 PUBG 灵敏度分离。gameModules.test.tsx 核验注册描述符不提前求值面板、只加载所选面板及 CS2 不声明瞄准设置；SettingField.test.tsx 覆盖两游戏不同频率范围、无活动游戏的全局 DPI、未知游戏与跨游戏字段拒绝。既有鼠标采集、模型确认、ESP 设置顺序、比例速度和状态栏回归继续运行。

这些是后台合成数据和组件测试，不替代真实鼠标回报率、DirectX 跨屏 DPI 或游戏手感的人工验收。当前没有新增 CS2/GTA5 数据能力，也没有要求 Reader 改协议或字段。

2026-10-22 隔离隐藏 Electron 验收已通过，报告位于 out/hidden-workspace-smoke-20261022/run1/report.json。覆盖新导航、游戏设置原38项事实、工作区创建/重命名/切换/同ID重载、失败撤销与明确恢复、无活动游戏的设备状态、旧代次拒绝、合成鼠标结果与活动区域稳定、渲染进程崩溃撤销。窗口全程隐藏/未聚焦，无截图、系统输入、真实采集或设备连接；验证进程和原生子进程已退出。DOM 导航 28 次测量 P95 约7.9ms，仅是隐藏窗口的DOM/布局，不代表前台绘制或真实游戏性能。


## PC2.1.1：READER 与数据看板

READER (/，保留旧 overview 启动兼容) 仅显示当前游戏 SourcePanel 的连接与订阅。数据看板 (/dashboard) 组合模块必需的 DataPanel，PUBG 的人物/FOV/目标/当前物品/观战和录制载入、CS2 的人物/武器/Build/血量/灵敏度/角度/punch均在这里。公共页面不再含 PUBG 兜底面板；两个游戏共用页面入口及导航，数据缺失保留占位，真实0保留。

上下文栏分别选择 reader / dashboard 事实；READER 不订阅盒子故障或人物角度计数，数据看板不显示连接表单。全局硬件三点和日志保留既有职责。原生高频数据/绘制路径未变。

CS2 订阅频率连接期间可改1–300Hz；原生同连接发新 Subscribe，匹配ACK前保持此前确认值，快速修改合并，拒绝则保留原订阅。PUBG 连接期间改频先保存，再用现有连接动作重新订阅 Living / Local，界面明确说明短暂等待新数据。游戏切换后晚到的提交不会触发另一个游戏重连。

CS2数据看板包含原生MouseMapping、Recoil时间/上下文/相机来源和条件眼角基准。只展示原生计算结果，不由JS计算/发送鼠标增量，不将数据可用当输出资格。上述PC2.1.1时尚无CS2控制器；PC2.2起已有独立原生控制和可选校准，Aimbot页面按模块真实能力装配。

PC2.2.4的Aimbot运行面板区分当前计算目标、按住期间保留的目标身份、当前暂停与最近一次暂停原因/年龄/次数。恢复后撤销当前警告，但不抹去诊断。明确死亡、真正会话/人物变化与真实松键不使用“缺组后自动恢复”的提示。响应核对来源由原生推送，JS不自行比较输入角或推断已执行鼠标报文，也不缓存几何补全失效帧。延迟/平滑优化只提供有据方案，数值参数未因用户口述公开方案而改变。


PC2.2.5历史基线：Aimbot页顶部按游戏懒加载AimVersions，用共用AlgorithmVersions与useSettingBinding提交稳定aimVersion。当时CS2提供v1/v2，PUBG仅v1；共用页面不分支解析游戏schema。版本说明逐项列出实际Source 2字段、参与/未参与因素与运动差异，支持中英文。未知版本拒绝，选择按游戏持久化；校准期间禁切版，切换清理旧意图，其他参数继续使用当前值。PC2.2.6在此基础扩展，UI只展示原生modeId/version/motionStatus，不计算后坐差分。详见docs/AIMBOT_VERSIONS.md。

PC2.8.33当前布局：Aim常规以`GameUiModule.aimGeneral`的稳定模块ID装配共享算法、移动、计数换算、时序或弹道设置；共用展示位于`features/aim/common`，游戏定义实际字段与可用策略。Aimbot仅保留手动部位/选人/范围/侧键，Rage和Triggerbot保留各自决策设置。`AimVersions/AimMovement`旧页面插槽已退役，通用页签框架不解释模块或消费者。原有设置键、保存上下文及控制数值不变，见docs/AIM_GENERAL.md。
