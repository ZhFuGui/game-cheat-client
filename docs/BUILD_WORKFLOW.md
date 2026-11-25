# 增量构建、验证和发布

2026-10-25。构建工具优化不更换当前运行的PC2.5.0，不连接设备或启动应用。正式VS仍为core/native两个项目，本地tests为第三个；固件仍使用其独立入口。

## 日常入口

在仓库根目录运行。目录迁移后的本机路径为 `G:\game-cheat-client`。

全新检出后先运行 `pnpm install --frozen-lockfile`。原生构建及打包需要按[固定资源契约](../third_party/traceline/README.md)准备 DLL 和地图缓存。本地解决方案、测试和夹具随开发目录保留。

本机旧目录迁移可使用 [PowerShell 脚本](../tools/rename-game-cheat-client.ps1)。把脚本放在旧目录之外，退出 Codex 和旧目录中的桌面程序后运行。

| 命令 | 行为 |
|---|---|
| `pnpm build:ui` | React界面及其类型检查，复用未变化的结果 |
| `pnpm build:desktop` | Web界面与Electron壳分别判定；UI小改不重建壳 |
| `pnpm build:native` | Release MSBuild增量构建；不执行Rebuild/Clean |
| `pnpm build` | 完整开发产物，C++由MSBuild增量，Web按内容缓存 |
| `pnpm check` / `pnpm test:changed` | 按上次成功验证后的内容变化选择检查；首次完整验证 |
| `pnpm check:all` | 完整覆盖；相同输入、程序与测试的成功结果可以复用 |
| `pnpm build:plan` | 查看下次源代码验证的范围 |
| `pnpm dist:dir` | 开发预览解压目录，不压安装器或ZIP |
| `pnpm dist:win` | 正式验收、Windows安装包和ZIP |

高频界面调试继续使用`pnpm dev`的热更新。无需为了每次文字、样式或局部逻辑变化制作安装包。

预览输出在`out/packages/<版本>/preview`，正式发布在`out/packages/<版本>/release`；这些入口不会覆盖正在运行的`out/desktop-release`，不会自动换版或启动应用。预览的增量验证不能代替正式发布验收。

2026-10-29用户明确要求分发包不包含许可证、NOTICE或版权说明附带文件。UI公共资源不再提供licenses目录，原生准备不复制LICENSE-NODE-API。标准builder的files规则在生成ASAR前排除这些文件；`tools/after-pack.cjs`清理解包目录中Electron等自动附带文件，并检查ASAR内部零残留。临时增量builder配置同样必须继承这两项，不能从旧配置复制后漏掉。只清理包内文件，图标、代码与运行数据保留。

## 本机生成产物清理

`powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/clean-project-space.ps1` 先预览；加 `-Apply` 执行。默认保留现用程序及目录连接的真实目标、最新一版打包目录、最近完整回退及其后的增量备份、最新安装器/ZIP。`-KeepPackages`、`-KeepRollbacks` 可增加保留数量，最少各一份。

脚本仅从旧程序包、测试程序和较早换版副本中选择生成的二进制/缓存文件；不移除整个项目目录，也不清理源码、Git 历史、用户配置、依赖、地图原件、当前构建缓存和诊断报告。不遍历文件系统连接；执行前再次检查完整路径、受保护目录和父目录连接。占用或访问失败的文件跳过并记录，不结束进程。计划清单、保护目录及执行结果保存在 `out/disk-cleanup/<时间>`。预览大小是文件逻辑大小，实际释放空间可能因硬链接与压缩而不同。

2.8.64 的现用 `out/desktop-release/win-unpacked` 为目录连接，真实内容位于 `out/packages/2.8.64/preview/win-unpacked`。两条路径都必须保护。清理不运行或重启 Reader。

## 构建与资源

C++的头文件、编译选项、工具链及对象依赖继续交给MSBuild追踪，不用脚本的源码列表代替它。`build.ps1`默认4个编译进程，可用`-CompilerProcesses`调整；同配置用独占文件锁避免两个脚本同时写对象文件。公共编译选项保持稳定；新增第三方依赖应限制在对应项目/源码项，避免无关文件重编译。

冻结Reader、CS2契约及traceline公开边界在core的一个构建目标中统一核验，直接在VS构建也生效；脚本入口不再重复跑同一检查。SDK摘要核验仍保留，未改动冻结协议或放宽版本匹配。

应用图标统一以`data/branding/application-icon.png`为原图，来源与摘要保存在同目录`source.json`。Web的ApplicationMark、网页图标及桌面ICO共用该来源；`prepare-desktop-icon.ps1`只做保留透明和比例的多尺寸格式转换。UI和ICO构建缓存均追踪branding目录，单独UI构建不生成ICO；包输入包含resources，替换原图或ICO会使旧包失效。启动和故障页不得手写另一套M图标，侧栏不恢复已删除的品牌。

2026-10-26用户批准改用紧凑构图的透明派生资源；清单记录原始来源摘要、派生方式与提示词，不能把生成式派生称为无损裁剪。原始资源保留在`.migration/icons/before-tight-20261026`，当前同尺寸预览在`out/icon-crop/before-after.png`。资源换版应同时核对EXE内嵌七档图标和独立ICO，运行程序正常退出后再替换，并更新已有快捷方式图标；不为图标变化重复转换或完整读取地图。

原生EXE、输入模块、CRT及运行清单只在内容变化时复制/写入。大碰撞场景日常只比较清单、目录和文件状态，未变化时不读取2.14GB内容、不重复复制。运行时每个实际加载的场景仍由原生适配器核验完整摘要和身份。

`prepare-desktop-resources.ps1 -FullVerification`可显式逐场景完整校验。正式发布在生成新包前完整核验一次；构建、准备、压包阶段不再各扫一遍。地图、DLL、目录或加载适配代码变化会触发原生全场景兼容验证；纯UI变化不会反复加载24张地图。相同输入与未变化的已验证包可直接复用。

## 验证选择与缓存边界

- UI/桌面TypeScript变更用Vitest实际导入关系选择相关测试；共享契约、公共配置或未知依赖保守扩大为完整范围。
- CS2专属原生变更运行CS2套件及真实桥接互验；通用/未识别C++依赖运行all。桥接测试开始前先完成Debug原生构建，不与同配置链接竞争。
- 工具变更运行工作流测试及架构检查；首次、共享边界改变或显式完整检查覆盖Debug/Release、Web、类型、lint与架构。
- 部分测试和完整测试使用不同缓存记录，部分通过不能冒充完整通过。失败、不完整输出、输出被改动或输入改变均不记录为可复用成功。
- 缓存存在`out/workflow/state.json`，只属于本机生成数据；`last-run.json`记录每步耗时、跳过与失败。小文件使用内容摘要，同一轮重复读取复用摘要；包目录状态使用本机文件元数据，正式发布完整资源校验仍独立执行。

强制跳过工作流结果缓存（C++仍由MSBuild判定，不自动Clean/Rebuild）：

```powershell
pnpm workflow build --force
pnpm workflow check --force
pnpm workflow package --force
```

缓存丢失或损坏按未验证处理。异常退出可能留下`out/workflow/active.lock`；先确认记录的PID已退出，再删除该单个锁文件。不会为清理锁而结束任何进程。

## 本轮测量

无改动原生构建约5.75→3.66秒；Web无改动约0.4秒；无改动验证约0.67秒；预览包重复调用约5.09秒且不再次压包。单个C++探针只改变一个对象文件，单个UI样式探针只重跑UI及相关类型检查。实际数值受机器负载影响，报告在`out/workflow/validation/summary.json`；探针源码已还原，未重启正式应用。
