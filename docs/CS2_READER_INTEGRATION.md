# CS2 Reader 接入

## PC2.8.47：接入 v2

运行客户端使用 v2，冻结 descriptor SHA256 为 `de589b63c5efe26492a267bafed709034f8ad31cec02990a3662a87eacaff727`。2026-11-19 对已配置 Reader 的正常 TCP 握手实测返回该版本和摘要。旧版 PC2.8.46 的 v1 校验在订阅前拒绝了它。新版先核对 Envelope、Hello 版本及完整摘要，再发送 v2 订阅请求。

`Status.reason_code` 使用 tag 5，缺省或 0 表示无详细原因。`Reject.reason_code` 使用 tag 4，要求正数并保留请求 ID。原因按 `Rxxx` 及本地说明展示，未知编号保持原值。订阅回执、等待数据、失效、停止各有独立语义。改频被拒绝时保留最后一次已确认的订阅频率。失效清除当前帧，只有后续有效快照恢复数据。源数据检查器使用 v2 descriptor，人物、武器等快照报文和时钟计算保持原语义。

原冻结 v1 文件和夹具留档。v2 回归以冻结 descriptor 生成控制报文，沿用同一合成快照原始字节。实测新原生宿主在隔离配置以 1 Hz 订阅已配置的 Reader，收到回执和快照。

## v1 历史接入记录

## PC2.8.27：完整 Inaccuracy 与独立 Spread

2026-10-30 经用户“好，开始”授权，源码从3d66骨姿态契约配对升级至f1905836aeff62574568e6534f866ba14bfee516b0d79a595b57b1412d748446，完整交付在R:/cs2/sdk/reader-v1-f1905836aeff-accuracy-complete。Reader负责当前版本的只读采样、原生分支与求值；本项目只消费公开proto/SDK/descriptor和夹具，不迁入其采集或内部算法。原tag10只是m_fAccuracyPenalty，不能改名为完整精度；后坐冻结求值器保持2050be...原字节。旧契约/SDK/夹具备份.migration/cs2-reader-before-accuracy-20261030。运行包最终状态以PROJECT_MEMORY的发布段为准，不能从此源码契约章节推断已重启Reader。

新增Weapon.accuracy_evaluation(tag15)与Hello.weapon_accuracy_evaluation(tag14)，模型ID为cs2.weapon_accuracy.14193.v1。两个结果各有status与有存在性的float value；只有VALID=1带有限值，有效0保留。普通失败如dirty velocity不撤销独立Spread；畸形的状态/值、未知模型、成功分支必需输入缺失或读区间不在外层本批读取内，撤销新增观察而保留原快照。完整owner与local controller/pawn/active weapon句柄关联；不只匹配低位索引，不沿用上把武器或重生前的数据。快照失效清空整组显示。模型身份检查使用已审阅model_id，不增加游戏build数字白名单。

CS2的READER → 数据看板 → 摘要最上方显示“原生散布”：完整Inaccuracy、独立Spread、各自状态/失败原因、原生分支、模型ID、已更新penalty、移动/空中/转向/方向贡献及实际组读取区间；说明放右侧ⓘ。二者都是无量纲方向幅度，不是角度、像素、计数或命中概率。源数据页由新descriptor再生成字段树，保留mode_raw、原生状态游戏秒和原始64位QPC。此版本仅观察。PC2.8.43 起，Triggerbot 的狙击枪判定复用完整 Inaccuracy 的现有核验规则，详见 [Triggerbot](TRIGGERBOT.md)。Rage评分的独立同期改动由另一窗口授权与实现，见CS2_RAGE_SCORING.md。

已消费21个官方新增样本及真实C++ sampler/serializer正文，验证有效0、独立失败、畸形组隔离、无owner与owner未知、时间越界/高64位和身份清空。消费端新增86项原生断言与UI3项；真实native/TypeScript管道验证新值和失效清空。首轮桥接把capture100的精度样本接在恢复capture2之前，递增门禁正确拒绝；修正的是合成测试的1→2顺序，未放宽生产时序。全量Web其余73文件通过，修正后的该桥接文件定向通过；合计74文件443项。原生Debug/Release all、cs2通过；完整正式发布记录与包验收见out/cs2-accuracy。

Reader研究574个离线原生getter算例、26项采集/隔离检查、483项交付核验与短时只读服务测试的范围互不等同，不是逐发实战命中保证。原生GetAbsVelocity dirty缓存、未适配物理分支、未知分派/规则/眼角会明确不可用，不用旧值或默认零填补。Reader按批共用实际cvar读取与必要关闭核验，无跨批缓存。不同场景的8–9ms不能与低负载截图直接比较；同口径低载ABBA旧均1.1323ms、新1.4809ms，增加0.3486ms，存在调度扰动，不能推广为满载或长期90Hz保证。证据R:/cs2/analysis/2026-10-30-inaccuracy/read-cost-abba.json。

## PC2.4.0：地图身份配对更新

2026-10-25，用户明确要求提供地图名，并在Reader窗口授权消费者准备好后配对更新重启。当前正式SDK为`R:\cs2\sdk\reader-v1-12883573c800-map`，descriptor SHA256为`12883573c8002e668bc7323c50b6cd61260387d01204267c4f3402818eff8038`。原0dd后坐/输入/眼角数学内容与recoil.hpp原字节保持，新增外层World.map_identity=13与Hello.map_identity=12；下方旧版本章节保留历史验证范围，不再表示当前协议摘要。

MapIdentity包含状态、64位generation和有存在性的名称/资源路径。仅AVAILABLE=1、gen非零且两字符串存在非空才可用；其他状态禁止携带名称/路径。资源路径是引擎标识，不能直接作为OS文件路径。generation是Reader观察代次，不保证识别两次采集间完全隐藏的同名退出重进。当前快照缺组/非可用/失效/断开立即清除旧地图，READY/Heartbeat不复活；地图可用与否不进入原Aimbot/RCS资格门禁。

五份协议/descriptor/contract与三份SDK头由`tools/stage-cs2-reader-contract.py`逐摘要暂存并备份旧源；`tools/check-cs2-reader-contract.ps1`固定新hash并检查SDK。官方37条记录（含64位、存在性、非法状态和生命周期）与4份独立C++编码样本已消费互验。真实Native TCP测试涵盖可用→过渡→缺组→失效→READY/Heartbeat→恢复→断开及PUBG隔离，不放宽旧hash握手。

全部原生Debug/Release、Web50文件331项、13架构测试/578源文件、typecheck、lint和隐藏Electron验收通过后，旧PC81552/native79988正常退出；Reader窗口自行正常替换72632并启动24212，新Reader文件SHA为`03b01c9c2ff4611e85d2551028378361fc3d713ce715ed892b744ca2ec56c542`。随后启动PC2.4.0 main74676/native37784，窗口最小化且未抢前台。消费侧实收`CS2 #3903`、地图de_dust2/maps/de_dust2.vpk/generation1，过去1秒70个通过递增序号核验的快照；Reader侧后续独立确认19851–19880连续30帧，target_hz=70且read_failures=0。

用户最新70Hz、v1/native、RCS已启用且绑定mouse:right等所有原值保留；76份数据备份、75份原字节一致，仅CS2 settings新增默认false的掩体扩展键。没有自动校准、采集、实机输入或固件改动。旧PC归档`.migration/releases/pc2.3.0-before-game-extensions-20261025`；成对回退还须恢复旧Reader和`R:\cs2\validation\game-extensions-20261025\userdata-before-2.4.0`，旧程序不能解析新增键。手动切图和同名重进尚未实机验证。

消费侧证据`R:\cs2\validation\game-extensions-20261025\final-verification.json`；Reader侧证据`R:\cs2\analysis\2026-10-25-map-identity\paired-deployment.json`。traceline仍为独立的未装配二进制依赖，见[扩展架构](GAME_EXTENSIONS.md)和[DLL审查](TRACELINE_ACCEPTANCE.md)。

## 历史：PC2.2.3/0dd控制分支

PC2.2.3，2026-10-24，当前消费者为Reader v1/0dd最终眼角分支扩展，独立CS2控制器和Aimbot页已完成。默认采用明确标注未实测的原生1:1输入公式，四向实测为可选验证；旧有效记录继续选择实测模式。校准读取与瞄准所需眼角/后坐条件解耦，真正控制资格不变。native_context不是会话身份；普通本地已核验source2输入值不依赖它，其余分支仍须同批核对，合法跨批切换不触发松键门控；真正的分支不兼容或必要数据缺失仍暂停，完整数据恢复后继续同人。真实游戏闭环需用户验收。唯一开发根：`G:\mousebox-aimbot`。下方PC2.1.4/e88章节保留当时只读验证范围，不能当作当前功能限制。

最新SDK为`R:/cs2/sdk/reader-v1-0ddc58a90521`，摘要`0ddc58a90521768c2505d79fe721c5baf63da535c6f8f67b066434dacbc47b6d`，80文件逐hash核验，冻结原始proto/descriptor与19组记录夹具；recoil.hpp原字节不变。新增4历史、5父变换、6委托、7世界旋转，保留已求值value及委托resolvedSource/sourceEntity/depth；零句柄按消息存在性保留。元数据不齐仅撤销基准组，后坐/骨架/整帧继续。所有分支只和所属人物同context的AimPunch组合，不重复变换或替换后坐所有者。普通输入控制目前限1–4分支；5–7可展示/预览，未核验局部鼠标变换时不输出。详见[控制器与校准](CS2_AIMBOT.md)。

## PC2.2.3分支依赖勘误

原生c76160预测路由在历史lane选择之前进入93d320；无视图委托/父变换时调用952870，直接复制Pawn.v_angle，不使用视觉选择器。限定普通本地有效Player和当前完整simulation/P/U时，source2不要求eye/punch标签相同。其他source、未知basis、必要状态缺失仍拒绝；原始标签继续展示，derived directionContextIndependent在C++/TS/UI明确标识分支规则。Reader仅撤除source2对无关selector变化的错误关闭核验，身份、指针、门控和数据核验保留。无缓存、宽限、旧帧拼接、协议字段或摘要变化。纯后坐数值求解、倍率、衰减、积分和预测参数均未改。证据R:/cs2/analysis/2026-10-24-direction-diagnostics及R:/cs2/validation/cs2-direct-input-20261024。

## 历史：PC2.2.2同契约方向采样一致性

消费者将native_context从持久会话身份中移除，只核对本批眼角和后坐组；不同context的simulation不比较单调性。Reader把原有eye/punch紧邻采样，只在缺失/冲突时对本批完整方向组立即重采一次；不拼接两次半组、不伪填标签、不沿用旧帧或无界重试。三proto、descriptor、0dd摘要与求值器原字节不变。正常匹配选择器仍在-1/0/1切换；任何最终缺组或冲突都如实输出，控制器暂停并在完整数据恢复后继续同人。Reader验证在 `R:\cs2\analysis\2026-10-24-context-consistency`，本窗口独立数据/回归/发布在 `R:\cs2\validation\cs2-intermittent-stop-20261024`；这些不代表物理跟随手感或长期零缺失。

## 历史：PC2.1.4/e88已交付范围

CS2 提供独立只读数据、矩阵 ESP、数据看板遥测、显示设置、数据设置及来源说明。每个游戏保留一份配置，侧栏直接选择 PUBG 或 Counter-Strike 2；既有配置ID、参数、校准、模型与记录保留。READER只显示连接及订阅，CS2遥测位于独立数据看板。

**CS2 没有注册自动瞄准程序，输入校准和设备后坐补偿输出未开放。** Reader 已在声明范围交付原生鼠标导数、后坐状态/纯求值规则和条件眼角基准；剩余的物理计数校准、CS2控制输出及实际跟随闭环属于本项目待实现与验收。复杂历史插值/委托/父变换分支保持未知。骨点是已核验骨原点，不等于命中盒。模型绑定可保存，但CS2没有采用模型输出。

用户授权与 CS2 Reader 的 `main` 窗口协调；协议先按该窗口的人类新指示暂停审阅，收到明确“好。开始吧。”及本窗口继续合作后再配对集成。本窗口未修改 Reader 源码、操作游戏或烧录固件；服务由 Reader 窗口正常升级。

## 协议与实现

- 正式冻结契约：`protocol/sources/cs2_reader`，三份 proto 与 descriptor。摘要 `e88ee47c39e30fd533964a63e8331301092b9e22cbae5accb232ebb9992f128f`。
- 来源：`G:\cs2-reader\protocol\reader\v1`；最终冻结 SDK 在 `R:\cs2\sdk\reader-v1-e88ee47c39e3`，正式部署信息位于 `R:\cs2\validation\recoil-14192\production`。上游交付注释中的候选阶段描述保留原字节，当前运行状态以验证记录为准。
- PC2.8.38起允许本机或远程IPv4地址，默认仍为 `127.0.0.1:13202`；服务为 R3 只读、4 workers。现有v1 Subscribe支持同连接在线改频，显式范围1–300Hz；Reader按活跃显式订阅最高值调整共享采集目标，无显式需求时默认90Hz。改频规则沿用PC2.1.1；骨架扩展已配对采用新冻结摘要，原字节来源与核验见文末。
- 4 字节大端长度加 Protobuf Envelope，正文 1..16MiB。版本/摘要握手、完整快照替换、失效/断流清空、事件与采样序号检查。
- 玩家、身体、武器按实例、进程 epoch、完整 32 位 handle 关联；控制器与身体分开。body 缺失不解释为死亡；当前 epoch 仍不保证切图递增。
- 当前人物、Local、武器共用一个实际采集区间，不伪造 Local/Living 两条独立时序。ClockQuery 往返建立 QPC 映射；心跳不刷新采样时间。
- 原生保留最新有界 protobuf Snapshot，不逐帧将实体、骨点或像素传给 Web。缺失清除旧值，真实零/false 保留；大整数留在原生或用字符串表示。
- 故障后按 5 秒重试。默认完整快照有效期 1000ms，工作区可设 50–5000ms；过期数据不继续作为当前画面。它不是侧键按住时长限制。

## 源码边界

```text
src/adapters/sources/cs2_reader/   # 独立 codec、订阅、时钟和接收/完成统计
src/games/cs2/
├─ model/                        # 独立人物、武器、Local、批次与存在性
├─ settings/                     # CS2 严格配置；不加载 PUBG 参数
├─ diagnostics/                  # 低频遥测与实际计算来源
├─ presentation/                 # 纯矩阵投影
└─ module_factory.*              # 游戏模块及应用端口组合
src/platform/windows/framed_socket.* # 共用有界 TCP 字节记录，不理解游戏协议
apps/native-host/composition/    # 工厂与全局设备绑定
apps/ui/src/games/cs2/           # 专用概览、ESP 设置与原理
packages/shared/src/games/cs2/   # 独立 Zod 设置契约
```

GameApplication 管理游戏自己的配置、状态、原理和绘制完成反馈；GameModule 负责生命周期、来源与不可变场景。只有全局设备白名单可跨工作区；CS2 不继承 PUBG 的目标、FOV、校准或算法说明。公共停止/设备命令不能被 game.* 命令覆盖。未提供专用回放时拒绝进入 PUBG 证据解析器。

现有设备所有者仍在旧 Backend 中，但仅由宿主通过 OutputBinding 暴露全局资格/停止端口，CS2 不依赖其类型、不打开第二条串口。当前没有绑定 CS2 OutputProgram；波特率、计数、预算与物理透传均沿用既有握手和固件。

正式 VS 仍为 core/native 两个工程，本地增加 tests。PUBG 的 Consumer v3 SDK 与原生计算不变。

## 显示与缺项

| 内容 | 当前行为 |
| --- | --- |
| 投影 | 16 元素行主序 world-to-clip 矩阵，左乘世界坐标；clip W 非正不投影，视口比例由用户匹配游戏 |
| FOV | fov_override 作为原始遥测展示；0 表示未覆盖，不当最终 FOV |
| 骨点 | 仅绘制本批VerifiedPartial/Complete的点及显式父子边；BoneRole独立于native index；未知模型/来源不可用不画骨架，部分组缺点不补齐。HEAD是骨原点，不是命中盒中心或头顶 |
| 距离 | cs2MetersPerUnit 默认 0/未知；显式设比例后才提供本地角色眼部到实体位置的米制距离，不能声称是真实第三人称相机距离 |
| 原始角度与求值 | 网络眼角、输入原值、条件basis分开；完整P/U按声明时刻求值，只有basis与punch原生上下文齐全且一致才组合分支方向推导，不冒充原生原子调用/未来开火；矩阵不二次叠加 |
| 开火 | shots_fired 是计数，不冒作实际开火布尔 |
| 输入与弹道 | MouseMapping导数已含符号/灵敏度/开镜倍率，不能重复相乘或除零；它不是物理设备标定，不复用PUBG弹道/输入模型 |
| 观战人数 | 现有数据不能确定则为空，不显示虚构人数 |

Data 设置只有本游戏的快照有效期和明确单位比例。旧 PUBG 的 general/ads/gravity/livingLocalSkew 等不会进入 CS2 配置。bootstrap 和事件再按 gameId 校验 schema，拒绝混合参数。

## 流量与时序

每个 CS2 订阅只有一条完整快照通道。原生统计实际 TCP 字节（含长度头/时钟消息）、近 1 秒接收快照数和来源到 GPU 完成的时间。消费者连接 ID 在原生进程内唯一，防止切回同一 Reader 时串入旧完成事件；同一采样的重复呈现只计一次。

GPU 完成延迟需真实 SourceSample 与近期时钟映射；没有 GPU 完成时为 null/等待绘制，不能用 CPU 投影冒充 GPU 完成或显示 0ms。绘制继续使用原有独立 DirectX 线程、最新场景槽与非阻塞呈现，没有固定 FPS 上限。

## 首次PC2.1.0验证与交付（历史）

- Release/Debug x64 构建及原生 all 通过；新增 31 项 CS2 官方夹具、存在性、大整数、UTF-8、坏帧、矩阵和配置检查。旧 PUBG/设备/Reader/工作区回归通过。
- Web 38 文件、255 测试通过，包含真实 C++/TS 管道中的 CS2 独立配置、版本消息、拆分记录、失效/完整恢复及切回 PUBG；类型、lint 和架构 454 源码检查通过。
- 8 秒独立原生只读接收 239 个不同 CURRENT 快照，持续 10 人/36 武器，时钟映射有效，接收计数约 30FPS。官方 Python Protobuf 5.29.3 独立复解实际原生导出的载荷：10 身体、200 骨点、36 武器、16 矩阵值，FOV 覆盖 0 的存在性保留。
- 使用用户现有数据的隔离副本执行隐藏 Electron 检查，覆盖 CS2 导航、设置、独立参数和禁用瞄准声明、原生子窗口及崩溃撤销；visible/focused/screenshots 均 false，未连接设备/Reader 或采集输入。
- 原生 SHA256 `3B71C90159515422DCDF2C041A66918B0B4FD41824AD909E4871B6F8866C6BF3`。116 文件发布到 `out/desktop-release/win-unpacked`，旧 PC2.0.5 位于 `.migration/releases/pc2.0.5-before-cs2-20261023`。本轮仅生成解压运行目录，没有新安装器/ZIP。
- 正常关闭旧 main68860/native49580 后备份 74 份业务文件。73 份旧业务内容保留，仅索引新增并选中 CS2；新建独立配置。旧 PUBG 当前 part=2 等用户最新值逐项保留。
- 新版 main78440/native90420 以 ESP 入口最小化启动。启动报告确认自动连接、本地 Reader ready、10 人/36 武器/build14192、30FPS，aimArmed=false、原始输入未采集。此时串口/盒子未连接；窗口最小化，未进行真实 GPU 画面、跨屏或游戏手感验收。

证据：`G:\mousebox-analysis\cs2-integration-20261023`，包括构建/回归、cs2-live-debug.json、实际 Snapshot pb 与官方复解、隐藏桌面报告、备份/发布 manifest、deployment.json 和 startup-pc2.1.0.json。

本机 Codex 文件映射使部分逻辑 `%LOCALAPPDATA%\Mousebox` 文件解析到包缓存中的同名目录；备份记录了各文件实际路径，部署脚本核对逻辑和既有缓存这两个明确目录。没有将用户目录移动到其他位置，没有借此放宽工作区写入范围。

## PC2.1.1 在线改频

用户授权双方协调后，Reader在原协议下接受新request_id的Subscribe，先完成已发送记录，再发送对应Subscribed；未开始的旧候选撤销，后续完成的真实采集才能成为新候选。消费者的request_id与ClockQuery共用单调序列，频率请求串行且合并快速修改；只由匹配ACK提交确认频率，不再用旧Hello.capture_hz限制新请求。明确请求的maximum_publish_hz必须等于请求值且不超过回执capture_hz。

期望频率保存为游戏设置；界面另列已确认频率和回执采集目标。它们不是完成率承诺，底栏接收FPS仍统计实际完整快照。改频Reject保留此前确认订阅和时钟并显示原因。3秒无接收字节进展才触发确认超时，大记录持续传输不会按总时长误判过期。断开或重连清除确认值、保留期望值；不生成重复帧或改写旧采样时间。

本窗口最终Release原生隔离只读验证，同一连接90→144→60Hz，近1秒窗口平均接收89.97 / 143.86 / 59.83FPS，人物数据持续有效，连接ID未变。临时客户端已断开，用户原订阅配置未改，无输入设备/前台/真实GPU提交验收。证据见 G:/mousebox-analysis/single-game-20261024/live-frequency.json；Reader最终PID8924/EXE SHA65942f6f5dd22e53b256330fa454f5cf41558a920a712e52fbf6232a328b0210及其自身动态验证由Reader窗口负责。

## PC2.1.3 正式骨架契约适配

Reader窗口的人类要求“按这21个主干骨点修改Reader，补齐骨点语义和父子关系，未知模型保持未知；通知aimbot对接，编译验证后更新并重启Reader”。本窗口通过read_thread核实此指令后接入已正式启动的服务，没有让上游新增其他字段或重启第二次。

新摘要b0788625c15642830740fb3e14906470d33d8f67c11b497c79a5f3575e47669e，三份proto/descriptor来自U:/cs2/sdk/reader-v1；13套官方合成消息逐SHA及长度前缀核验并加入跨语言契约检查。旧冻结件/夹具另保存在外置证据contract-before/fixtures-before。

Hello能力与每个人的Skeleton.status双重验证。模型profile为cs2.agents.main_body.14192.v1，21点20边；角色和native index独立，例如LEFT_UPPER_LEG的role16对应native17。已核验组要求当前模型/profile；未知、不可用及缺组不保留几何；缺失角色不推断。拒绝坏枚举、非有限坐标、重复点/角色、自环、多父、环和悬空端点。单一ProjectionState中完整→部分缺头→未知→完整回归证明21/20/0/21点、20/19/0/20边替换，不沿用旧头点或旧边。

Debug/Release原生all通过，CS2检查88项。Release独立只读8秒得到241个不同CURRENT批次，每批10人、10完整骨架、210点/200边/210显式角色，末窗30FPS、时钟映射有效。官方SDK Python/protobuf独立复解原生载荷得到相同结果。Reader监听127.0.0.1:13202/PID71500；临时消费者已断开。仅验证协议、CPU投影及存在性，不宣称真实GPU/游戏效果。CS2瞄准、输入校准、后坐补偿仍未开放。

证据G:/mousebox-analysis/cs2-bones-20261024，最终发布与数据保存状态见PROJECT_MEMORY。

PC2.1.3正式解压包131文件、43项UI资产与ASAR main核验通过，原生SHA f604b4cbe949f1c735e8eb8528e4d3a174bd2a3dfdf0f20fe7f82896c7702ff5。当前main4768/native52512最小化，自动连接CS2 ready、10人物、期望/确认30Hz、实际30FPS；采集未开启、aimArmed=false。换版前76份数据全部逐字节保留；原2.1.1归档.migration/releases/pc2.1.1-before-architecture-cs2-bones-20261024，快捷方式目标及参数不变。最终295项Web与11项架构守护通过，隐藏Electron检查visible/focused/screenshots均false。没有新的安装器/ZIP或真实游戏效果验收。

## 2026-10-24 待办：CS2 Aimbot 接入的上游核验

用户要求先检查完整条件；有缺项就停止接入。核对当前正式b078契约和Reader最新离线研究后确认：骨架语义/拓扑已齐，但输入计数到横纵角度及开镜倍率尚未核验，完整后坐求值所需状态/游戏时间也未交付。CS2控制器与Aimbot页属于本项目待开发项，不将这些实现工作误报为Reader缺字段。

随后用户明确授权“把这些缺项发给cs2-reader，让他继续核验补齐，齐了再接入CS2的Aimbot页”。已向Reader main线程01a16878-105c-7890-b361-b3089f9b1189发送完整需求，要求区分已读取未发布、尚未采集和规则未核验：

- 输入：当前生效m_yaw/m_pitch、sensitivity、轴向/反转与开镜/变倍倍率、FOV或zoom设置影响范围及验证依据，不能把常见默认值当当前实测。
- 后坐：predictableBaseTick、predictableBaseTickInterpAmount、predictableBaseAngleVel、unpredictableBaseTick，以及相机CsViewPunchAngleTick/TickRatio；也可交付同等核验且语义更明确的求值结果。须明确游戏时间/子tick与QPC关系、会话与有效期、10–100ms执行延迟下可外推范围。
- 语义：射击方向、输入视角、相机反馈、准星/观战分别采用哪个入口、组合/缩放及不重复补偿条件。shots_fired/recoilIndex/accuracyPenalty不能自动冒作实时开火状态；若依赖实际开火须给出已核验判据或说明缺失。
- 交付：build/证据、正式proto/descriptor/hash/SDK、单位/轴/时间域/存在性和0值、合成夹具/完整替换验证及各场景适用范围。分析资料遵循该窗口人类要求放R:/cs2。

资料未齐不修改CS2输出资格。若更新正式schema，先交付冻结候选供消费者配套，再协调服务切换；本窗口不直接改Reader源码或自动操作用户桌面。PUBG协议和现有运行版保持。此条是待办与授权记录，不表示缺项已完成或功能已上线。

Reader已确认继续核验；时间转换/native_context与输入映射尚未完成，正式b078服务保持。用户要求删除周期任务，cs2-aimbot自动化已删除，不能恢复定时轮询。已要求Reader完成正式核验/冻结交付后，主动向本窗口01a154b4-7ce0-7b33-af06-b967eeaaf8f7发消息触发接入；必要阻断也直接反馈。CS2接入目标继续，资料与验证齐全后才实施，不表示当前已开放。

## PC2.1.4 最终原生状态只读接入（配对换版中）

最终SDK在R:/cs2/sdk/reader-v1-e88ee47c39e3，摘要e88ee47c39e30fd533964a63e8331301092b9e22cbae5accb232ebb9992f128f；中间62仅用于离线原始人工记录和早期配对。当前结构化AimPunch/CameraViewPunch、原生MouseMapping、camera完整来源、input_eye_angles/eye_basis及native_context已接入。冻结SDK纯计算头在third_party/cs2-reader，只由games/cs2/evaluation包装，避免将传输代码或设备调用引入计算。

EyeAngles的网络字段、Pawn输入原值与已核验basis严格分开；只在basis及AimPunch上下文均存在且相同、来源已核验时显示符合该分支的方向推导，不宣称直接调用了原生函数、逐字段原子结果或未来实际开火方向。未知/历史插值/委托/父变换不回退。P/U可独立展示；相机按来源人物及本帧视觉时间计算，ESP矩阵不再叠加，观战来源变化没有跨帧角缓存。

原生all与299项Web、架构/类型/lint、最终候选8秒241帧/30FPS及新SDK独立复解通过；完整物理设备校准和CS2跟随闭环仍未完成，不注册aim能力或打开Aimbot页，不把只读接入包装成已可自瞄。数据看板展示源状态、时间、上下文、原始系数与有条件派生值，中英文齐全。

131文件最终包已备份原数据后准备到正式目录，旧2.1.3完整保留；当前等Reader正式端e88确认再启动PC。最终运行/用户数据核对状态以PROJECT_MEMORY最新条目为准。

## PC2.1.4 已发布的最终状态

最终契约e88ee47c39e30fd533964a63e8331301092b9e22cbae5accb232ebb9992f128f，SDK R:/cs2/sdk/reader-v1-e88ee47c39e3。新增眼角基准/输入原值及原生上下文，只有明确来源、两组ctx均存在且相同才能组合；未知/复杂插值/委托/父变换不回退。新DTO、数据看板及原理说明区分网络原值、条件basis与各路径纯求值。原始网络角加P/U不是武器服务方向或未来开火角。

最终Debug/Release原生all、225求值/映射+99协议、Web43文件299测试、类型/lint/477架构边界及11守护通过。e88候选只读8秒241帧/30FPS，官方新SDK复解16项眼角字段一致；192机器码案例最大误差0.000198364度。旧62人工01–06+08独立8859帧审计另保留，07失败记录排除。

131文件包与ASAR/43UI资产/nativenode资源核验，native SHA f393eee8220c7f2472cb3c0b1c1c94d0f15d8842e084b2052d1d20db5fd5a6db。关闭旧PC后备份76文件、原子替换目录，等Reader正式e88确认再启动；旧2.1.3完整保留。正式Reader PID87936，R3/4workers/read-only；PC main39732/native88324最小化，原CS2配置与30Hz/实际30FPS/10人正常，aimArmed=false、canAim=false、采集未开启。76份文件换版/启动后逐字节不变。

这仅是只读数据、诊断与纯求值接入，CS2设备计数校准、控制输出程序及实际跟随闭环仍属本项目待完成事项，Aimbot页尚未开放；不以未实现控制器冒称上游缺字段，也不自动开启输出。没有新的安装器/ZIP、实机GPU或跟随验收。完整证据与回退路径见PROJECT_MEMORY最新条目。

正式配对已收尾：Reader候选17200经命名事件正常退出，13212不再监听，正式87936/13202保持；Reader记录30Hz/1条消费连接/10人物/读取失败0。上游收尾记录R:/cs2/validation/recoil-14192/production/paired-verification.json。本项目启动后76文件逐SHA全不变，无输出或采集开启。
