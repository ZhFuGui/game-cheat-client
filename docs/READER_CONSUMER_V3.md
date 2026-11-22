# Reader Consumer v3 接入

2026-10-23，PC2.0.3已部署到本机原发布入口，并连接正式Reader v3。Reader / Radar 的远端切换由 Reader 窗口协调，本窗口不重启它们。下方2026-10-22候选结果保留原验证范围，不将通信通过等同于持枪或游戏效果通过。

## 2026-10-23 本机换版与真实连接

用户新指示“和新版的Reader对接一下”恢复封存工作。确认SDK / descriptor没有变化，116候选文件完整校验后，通过正常关闭旧软件、备份现有设置、保留旧发布目录再替换的方式部署。发布入口仍为 `out/desktop-release/win-unpacked/mousebox-aimbot.exe`；版本2.0.3，native仍为下方记录的6b15摘要，快捷方式无须改动。旧版保存在 `.migration/releases/pc2.0.2-before-reader-v3-20261023`。

正式 Reader 通过局域网的 13102 端口提供数据。先完成8秒独立只读TCP，再用实际桌面客户端完成20秒有界状态采集；临时客户端已退出，桌面保留最小化运行。实际两条流各保持一个会话、均为current，Living收到16–17人物，人物和相机数据已接通。用户60Hz设置保持；近1秒FPS读数均值Living35.947 / Local54.632，不能声称60Hz达标。19份有效诊断中，Living批次时序17applied、1启动不可用、1不确定，Local18applied、1启动不可用。另1次诊断读取失败不等于协议丢帧。

本轮Reader正式上报has_current_item=false、无weapon_token/name，已对实际PB复解核实；native aim basis不具备当前值。没有自动控制鼠标、按键或开枪，不能宣称弹道、后坐或手感已验收。最小化时未提交可视绘制，不把waiting-draw当成协议错误，也不伪造绘制完成耗时。

当前配置与校准等7份实际业务文件前后SHA一致，既有模型/采样不移动、不删除；左上角纯文字改动已发布。详细收据、启动报告及状态分布位于 `G:\mousebox-analysis\reader-v3-connect-20261023`。以下为前一日冻结接入、离线验证和候选交付记录；“未部署”只描述2026-10-22当时状态。

## 冻结输入与构建

正式上游交接目录为 `U:\pubg\analysis\reader\consumer-debug-skeleton-20261022\protocol\sdk`。本项目已导入其完整文件，构建无需继续依赖该外部目录。

| 项目 | 固定值 |
| --- | --- |
| wire / clock 版本 | 3 / 1 |
| `SDK_MANIFEST.json` SHA256 | `af09ae0f45189ff7b544e1ca96c3c0daf663e43b8de0c1ea751d5d063b4e463b` |
| descriptor SHA256 | `4e5f26fed497f86a37d686df443be54d42dcc5e1eeeae79143a19f209e5f6f92` |
| 自有 `SOURCE_MANIFEST.json` SHA256 | `d10e21b2f7ae918cb0316adb9cb804378c11441300b04b4bf520ec0edf273288` |
| 自有 `client-sources.props` SHA256 | `1f97d270e478fe99cb2f999240bc277b2438884eb38f3e47f8373940b30d6df3` |
| 上游完整范围 | 99 文件：86 src、12 proto、1 protocol README |
| 编译范围 | 17 client CPP；11 provider CPP 保留原文件，但不编译 |

`third_party/reader-consumer` 保留上游文件原字节。`protocol/sources/pubg_reader` 镜像正式协议与 descriptor。`projects/mousebox-core.vcxproj` 只导入显式 client 编译项，不使用整个 SDK 的 CPP 通配；对象路径保留来源目录，防止同名文件覆盖。

`tools/check-reader-contract.ps1` 同时核对 SDK 全量清单、源码、client/provider 闭包、编译项、公共 proto 镜像及适配器摘要。`tools/sync-vs-filters.py` 识别该显式导入，VS 浏览目录不改变实际编译边界。不能只更新摘要来掩盖上游文件的本地修改。

## 运行语义

- TCP `Hello` 使用 Consumer profile 1 / features 0；Consumer 协商读取 `business_kinds`，不借用 Debug 的 Native kind 列表。
- 在线入口使用正式 Consumer `State`、`ReceiverSession`、Baseline 19 和 Update 20；wire v2 会被拒绝。
- Living / Local 使用独立会话、基线、时钟映射和失效状态。业务身份按正式 Reader 实例、epoch 与不透明 token 关联，不能每连接重造，也不向 actor / index / serial 填假值。
- 自有模型区分 NativeLifecycle、EntityToken、ControllerToken、WeaponToken。原生地址比较只对实际 Native 身份生效，避免把全部无原生 actor 的 Consumer 人物误当成本地人。
- 骨架使用正式 role / parent_role 和 owner / layout / pose 绑定，动态骨位置保留真实 pose 来源；不将语义 role 冒充原生 bone index。头部圆环继续使用世界尺度、相机深度和实时 FOV 投影。
- 完整身体世界速度使用正式独立观测路径。能力声明和当前读取成功是不同概念；缺失、CLEAR、合法零值及读取失败分别处理。
- 当前武器、recoil、trajectory、controller 和 native aim basis 按各自正式来源核验。`native_context_token` 用于资源上下文身份；动态归零修正角保留实际值和来源，不加入身份，也不重复叠加。
- 原生角度资格保留当前 profile、归零、来源宽度、样本时间及跨流匹配规则；本轮没有调快控制、放宽时间阈值或改变侧键、死亡及缺帧策略。

## 来源时间

来源区间采用上游实际依赖范围，不人为缩短为某个读取点，也不使用数据接收时间替代采样时间。

| 数据 | 来源要求 |
| --- | --- |
| Weapon token / name | `LocalWeaponSample.observation` |
| 当前槽位、存在标志、owner | 选择读取的真实较宽区间 |
| muzzle、recoil、trajectory | 分别保留自己的观测 |
| ControllerRotation 各字段 | 独立 controller / rotation sample |
| AimBasis 计算值 | 实际计算依赖区间；controller 字段保留独立 sample |
| Skeleton 世界位置 | 实际 pose sample；owner / layout / pose 绑定保留各自来源 |
| Context origin | 允许正式显式 reused，保留原来源；相机 origin 必须与其值和来源一致 |
| 动态相机、controller、basis、速度 | 当前 capture / category，不以静态 reused 来源绕过资格 |

Consumer 不传的 Debug verified 起止时间保持缺失。展示原始来源起止时间不等于拥有 Debug 核验区间。诊断导出的身份明确标注 kind / token；没有传输的原生地址、索引和 serial 为 null。

## 回放与诊断边界

保存的 PB 保留完整协议值，Consumer JSON 证据是标明语义的摘要。`recorded_codec` 仅在文件回放入口显式将原有 v2 丰富记录按同一值结构交给 Debug 验证，保持原身份、时间和 profile；在线 Session 不调用此入口。既有冻结二进制夹具没有被替换成伪造的新样本。

历史文档中的旧 descriptor、CPP 数量和线上结论仅表示当时版本，不能用于当前冻结核验。旧 SDK / proto 已保存在仓库外备份中，不恢复为当前编译入口。

## 已执行验证

外置验证目录为 `G:\mousebox-analysis\consumer-v3-20261022`。所有下列验证使用隔离配置、合成或既有离线夹具，没有设备输出、桌面输入或真实游戏操作。

| 验证 | 结果与证据 |
| --- | --- |
| Release / Debug 精确最终源码构建与原生 all | 通过；`build-v3-final-*.log` / `native-all-v3-final-*.log` |
| 不透明身份、原生资格、语义骨架 | 59 项通过；`domain-tests-final.log` |
| Consumer 正式 codec / session / projection | 25 项通过；`consumer-tests-final.log` |
| TypeScript / React / 桌面管道 | 35 文件、233 测试通过；`web-tests.log` |
| 类型检查、lint、架构检查 | 通过 |
| 实际 Release CLI 双连接 TCP | 通过；`tcp-result.json` |
| 实际打包 native 双连接 TCP | 通过；`tcp-packaged-result.json` |
| 实际打包 native stdio 生命周期 | 通过；`packaged-bridge-smoke.json` |

TCP 对端使用 Python 3.11、官方 protobuf 5.29.3 与正式 descriptor；Windows QPC 作为真实本机时钟。两条独立类别连接各完成 2 次基线 ACK、1 次失效 ACK、16 次心跳、3 次时钟探测；包含长度前缀分片、更新、失效后新基线恢复。它是实际跨进程网络验证，不能等价为远端 Reader 或游戏实测。

打包 stdio 验证使用新隔离数据目录，完成会话建立、状态读取、38 项算法说明、续租、撤销和关闭；Reader 未运行、盒子未就绪、未采集，进程正常退出。SDK 有两处上游 C4456 遮蔽警告，未改冻结源码；自有代码保持严格警告检查。

## 候选交付与未完成项

- 候选：`out/desktop-v3-candidate-2.0.3/win-unpacked`，已包含左上角移除品牌图标、仅保留项目文字的改动。
- 候选 native SHA256：`6b15aa848c05dd73ca97bf9a1bda641255896628651c2cef28381cb35925e779`，与最终 Release 一致。
- 本轮仅构建解压运行目录，没有构建 PC2.0.3 安装器或 ZIP，没有替换正式目录或桌面快捷方式。
- 最终文件哈希、验证收据和状态边界见候选目录外层的 `candidate-receipt.json` 及外置同名收据。
- 未进行真实 Reader v3 现场连接、游戏效果、前台 DPI / 输入或硬件验收；未烧录、未修改 Reader、未修改固件 v7。
- Reader 已要求本窗口先给最终本地收据，不自动启动软件或设备。配对切换与实测留待后续明确协调，不能把当前候选描述成已上线。
