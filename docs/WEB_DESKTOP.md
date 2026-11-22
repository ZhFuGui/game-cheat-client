# Web 桌面开发与交付

环境：Windows x64、Visual Studio C++ v143 / Windows SDK、Node 22.12+、pnpm 10.18.2。运行发布包不要求系统 Node 或开发服务器。Electron 固定 44.7.2，其余前端依赖参照 DIMO 锁文件固定。

```powershell
pnpm install
pnpm build
pnpm typecheck
pnpm lint
pnpm test
powershell -NoProfile -File tools/test.ps1 -Configuration Release
pnpm dist:win
```

`pnpm dev` 启动本机 Web 开发服务和桌面外壳；它不会自动连接 Reader/盒子。正式包中的界面是本地静态资源，不开放 Web 服务。生产 C++ 解决方案两工程，本地三工程。测试命令使用合成数据和隔离配置，不自动实测校准。

发布包在 `out/desktop-release/`，含安装包、zip 和 `win-unpacked/Mousebox.exe`。PC1.1.10起默认数据目录为 `%LOCALAPPDATA%\Mousebox\`，随当前Windows账户解析，不依赖安装目录。`--data-dir=<目录>` 是完整隔离运行入口，不读取或迁移旧用户配置。旧 `--settings <ini>` 在相邻独立 Web 配置目录导入，该 INI 不被修改。

旧导航入口 `--esp`、`--learning`、`--controls` / `--aimbot`、`--aimbot-algorithm`、`--appearance`、`--settings-section <分类>` 和 `--minimized` 保留。`appearance/language` 映射常规，`device` 映射固件。原生工具统一为 `mousebox-native controller|reader|replay`，参数和退出码沿用原命令；只读命令不构造桌面业务后端或串口。

`settings.json` 由 C++ 原子写入，`web-ui.json` 由 Electron 原子写入。首次识别普通 Local 与 Codex LocalCache 两个旧来源，同 SHA 去重；不同来源需显式选择。INI 与用户原始采样/模型保留，备份位于 migration-backup。已有 Web 配置优先，不被首次导入再次覆盖。

默认目录首次使用且没有settings.json时，先恢复原 `%LOCALAPPDATA%\FuGui\Mousebox\` 中已有的Web配置；普通目录无Web配置才检查旧Codex LocalCache目录。只复制所选来源的settings.json、settings.ini、web-ui.json、human-motion、human-speed和migration-backup，不混合另一份来源、不复制浏览器缓存或锁文件，旧目录保留。逐文件核验且不覆盖不同内容的目标文件，settings.json最后写入；遇到冲突报错保留原件，可恢复内容相同的未完成复制。迁移来源和复制记录存data-directory-migration.json。新目录已有settings.json时始终使用新目录，不再重新导入。只有旧INI时保留明确选择来源流程，同时复制该来源的采样、模型和备份。隔离诊断不访问这些旧目录。

`--smoke-report=<path>` 必须配合 `--data-dir`，整个窗口保持隐藏；验证真实 renderer/preload/main/native 握手、跨进程 HWND、渲染进程崩溃撤资格、关闭清理，不连接设备。报告只证明该验证范围。

界面设计参见 `apps/ui/README.md`。按用户要求，分发包不包含许可证或NOTICE等附带说明文件。不包含 DIMO 学校业务、账号、MCP、更新身份或凭据。

## PC1.1.11 设置组织

设置分类依次为常规、固件、游戏输入、Aimbot算法、Aimbot鼠标移动、校准与诊断、显卡与绘制。Aimbot页只保留跟随参数和运行状态。原38项来源说明分为算法26项、鼠标移动12项，搜索、阶段过滤、展开原理、复制全部说明及按需订阅保留；鼠标页包含原生方向到二轴输入的映射。旧--aimbot-algorithm以及旧Aimbot算法链接转到设置算法栏；新增--settings-section aimbot-algorithm / aimbot-mouse。

重复“保存参数”和没有真实分级数据的日志级别下拉移除，参数/校准仍自动保存，日志保留搜索/清空/导出。手动端口检测移到诊断，实测校准与换算方式置于高级折叠区，明确实时FOV模式不使用保存的实测记录。普通实时FOV视角合并第三/第一人称显示，但保留原有存储值，载入页面不改配置；实测模式继续区分两种记录。DPI、手动灵敏度模式、原生/相机归零和学习模型参与范围如实说明。Reader已确认正式协议没有可自动分派general/aim/ads的已核验字段，本轮保留手动模式，不从FOV或方向分支猜测。

Reader失败后的两条流重连、系统TTL扫描、盒子失败/确认缺回传后的重试统一5秒，首次可用时立即尝试；协议心跳、时钟探测、帧资格、150ms侧键新鲜度、实时处理和绘制刷新不属于连接重试。手动断开、诊断隔离和退出取消仍有效。

五档上限由Native speedPolicy直接发布speedPresets，前端下拉显示每轴600/800/1600/2400/3200计数每秒，鼠标说明页同时显示加速度和单步上限。它不是像素速度或固定角速度，不更改控制算法。轻微动作变化仅模型启用时可操作，切换时更新同一活动模型并取消本次按住，不再悄悄停用模型。

## PC1.1.12 鼠标移动调试

原速度学习移至侧栏底部“鼠标移动调试”，紧邻运行日志上方；原功能保留为速度学习子页。新增鼠标特征：手动开始与结束的本机Raw Input活动区域，结束后原生后台统计每条报文的计数/时间特征。结果包含计数分布、时间段最小/最大/平均曲线、接收间隔分布、分位数和峰值，可选择总计数或X/Y轴，并自动保留原始CSV及统计JSON。原始事件不逐条跨桥。操作、口径、容量和验证范围见[MOUSE_DEBUG.md](MOUSE_DEBUG.md)。没有Reader字段/协议变化或固件更新。

保留--learning和旧page=learning入口，均映射新调试页的速度学习子页；新增--mouse-characteristics直接进入鼠标特征。默认路径下的新记录位于%LOCALAPPDATA%/Mousebox/mouse-debug。沙箱preload的事件白名单由完整Topic映射做编译检查，订阅初始化失败明确显示错误并撤销已注册监听，不停留在启动等待。

隐藏--smoke-report检查新导航、两个子页和结束按钮；可配合--smoke-mouse-fixture=<原生测试生成的统计JSON>，仅在强制隔离且隐藏的烟雾路径将合成统计展示给React，用于图表排版检查，不启动Raw Input或注入输入。普通启动不走此夹具分支，最终报告明确标记syntheticCharts。

## PC1.1.13 采集区域稳定与状态点尺寸

鼠标调试页固定滚动条占位；隐藏旧结果、完成滚动位置调整并等原生区域确认后才发出采集命令，避免开始后布局变化触发正常的区域撤销。失败恢复旧统计，区域确认期间卸载页面不会补发开始命令。原生实时工作、输入线程和光标所有权规则未改。

底部三点直径9→11.25 CSS px（1.25倍），实际相邻圆点的空隙17→20.4 CSS px（1.2倍）；外层18px命中区域保留，通过状态栏gap13.65px实现，顺序、颜色和含义不变。隐藏验收检查计算样式，并用三种窗口尺寸与滚动后启动流程验证布局；合成开始命令被主进程截获，不触发真实采集。

## PC1.2.0 全仓解耦

桌面入口、应用装配、命令路由、事件发布、迁移与启动任务、隐藏验收分开维护；shared契约、Web领域组件/事务hook/状态投影以及有序CSS模块各有明确归属。原生宿主、会话请求、应用业务、后台文件队列、日志、Reader投影、显卡枚举和平台字节传输同步解耦。完整文件树、所有权、边界与测试入口见[架构说明](ARCHITECTURE.md)。保留旧导出和CLI/配置格式，不扩充工程数量，不改变控制与设备资格规则。
