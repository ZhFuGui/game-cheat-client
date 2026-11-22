# Reader 本机直接订阅

PC0.5.6已同步2026-10-13 CharacterMovement正式契约：新增组21及三位掩码7，消费快照30文件、descriptor SHA256 `6F76B83E1191C55E923BBFE8D6F6AE1E57DDC0D0FF5021D96FC71EC9540EB611`。零组件序号/原始引用/速度有效；失败清当前速度，实体/Spatial/骨点保留。四锚点现额外提供短期偏移和不确定度估计供[时间补偿](TIME_COMPENSATION.md)，单程仍不能精确测量。本机离线兼容验证不代表旧远端已支持组21，须由Reader升级服务端。本项目没有代为升级远程Reader或增加远程模块。

下文是0.2.1只读客户端与2026-10-12的历史实机验收，验证范围按原文保留；“本版不估计偏移”等表述仅对应该历史版本。

版本：只读客户端 0.2.1（观察记录增强） / 架构 0.5，2026-10-12。远程只部署 Reader；本机直接连接 TCP 13102，Living/View 各自独立订阅。本轮没有启动控制程序、访问串口、修改固件、接线或 Reader。

## 入口和使用

标准 VS 解决方案 `pubg-mousebox-aimbot.sln` 新增 reader-consumer 静态库、reader-client 只读程序和 reader-tests 测试，共八个工程。默认启动项目仍为 mousebox-controller；观察数据时须将 reader-client 设为启动项目。PC 构建不烧录固件。

正式只读 Release 路径：`G:\pubg-mousebox-aimbot\out\x64\Release\reader-client.exe`；Debug 在对应 Debug 目录。运行本机观察不需要盒子或串口。

```powershell
./tools/build.ps1 -Configuration Release
./out/x64/Release/reader-client.exe --host 127.0.0.1 --port 13102 --hz 30 --seconds 20
```

上面地址是当前目标，须在 Reader 确认新版本已部署且监听就绪后使用，不能连接旧的同机契约服务。`--seconds 0` 持续观察，Ctrl+C 退出；`--no-probe` 保留本机时钟声明但关闭诊断探测；`--no-clock` 不声明消费者时钟且关闭探测，数据仍正常安装。

单连接重连验证：增加 `--reconnect-living-after 10`，第一次 Living 握手完成的连接运行约十秒后，本机只关闭 Living socket 一次，再按正常退避重建；View 继续订阅。该选项不修改 Reader 配置，不新增控制协议。

程序只发送 Hello、精确 AppliedAck、InvalidationAck、Heartbeat 和可选 ClockProbe；没有 CaptureRate SET、串口输出、按键触发或合成鼠标运动。Hello 按正式订阅协议请求目标上限 30Hz，Reader 按所有消费者聚合调度。

## 冻结契约与来源

- Envelope 版本 2，clock_contract_version 1；Hello 旧 field 5 保留不用。消费者时钟可以缺失，声明时 Accepted 必须完整回显。
- 10 个正式 proto、libprotoc 35.1 的 descriptor 与 manifest 固定在 `protocol/reader/`。
- Descriptor SHA256：`D25C25F3E018B47068A408DCD96F7053BA4C7EDE32919198B0939B7F78AE1572`。
- `third_party/reader-consumer/` 固定上游纯消费依赖 29 个文件和逐文件哈希，采用 Reader 原生严格 protobuf codec 与 ReceiverSession，不依赖来源仓库绝对路径或 C++ protobuf runtime。来源与更新规则见该目录 README。
- MSBuild 前运行 `tools/check-reader-contract.ps1`，检查 descriptor、proto、源码快照和 schema.hpp 对应关系。

源码快照来自上游工作树的已确认契约，不冒称正式提交。远程 Reader EXE 的部署哈希和实际运行身份需另行记录；源码存在不代表远程部署完成。

## 行为与边界

Living/View 各自管理握手、双向记录序号、会话、固定基线、不可变当前状态、ACK 和失效。完整重建及核验后才 ACK；增量始终相对固定基线重建。同 token 重投只回 ACK，不重装状态或刷新已安装样本的接收时刻。

失效立即撤销对应类别，确认保留失败采集下界；旧基线不能恢复资格，恢复必须跨过 revision/capture/fence 下界。断线或协议错误销毁当前状态、基线和探测，重连使用新会话与新基线；退避 100/200/500/1000/2000/5000ms ±20%，健康五秒复位。协议不一致当前采用关闭并重握手恢复，没有另行实现同连接 ResyncRequest。

TCP 使用四字节大端长度，支持半包与粘包，先核对协商长度再分配，检测真实字节无进展。双向心跳只有对应 nonce 的回复完成挑战；普通数据流量不能代替本机心跳回显。网络与数据处理不等待串口。

源 capture/read_finished/send_started 原始 tick 保持 Reader 时钟域；本机只用自身 QPC 管理接收、超时和诊断。采用上游 source_clock_is_local=false，源内时序和本地提交前后不倒退分别核验，不拿 Reader 采样和本机 now 比大小。

0.2.1 新增 `--evidence-dir 路径`，每连接有独立证据目录，保留最后的实际 Accepted/基线/增量/探测正文、发送 ACK 包，以及核验后状态 JSON；增量与其最后基线配对，数量有界。每秒输出一次状态，LOCAL_CLOSE/STOP 额外保留结束计数；JSON 来源时刻标为 Reader monotonic_ns，不与本机 now 比较。证据仅用于历史验收，不是运动状态输入。

探测每连接至多 1Hz、一条在途。t0 封装后首次 send 前盖戳，短写不刷新；t3 完整正文接收后解码前采样。净 RTT 按各域频率换算两段时长后相减，显示 probe_id、t0/t1/t2/t3、样本年龄和 net_rtt_ms；跨机单程延迟显示 unknown。本版不估计单程、偏移或未来漂移上界。诊断失败或超时清掉诊断样本，不撤销有效业务数据；破损 Envelope 或会话序号错误仍关闭连接。

日志每秒显示 Reader stream/session、类别 epoch/revision、实体数、源采样锚点、本机接收后经过时间、基线 ACK/增量/失效/心跳计数，以及 ViewSource/ViewPose 状态。local_since_receive_ms 不是完整端到端数据年龄；空实体不能代替实战骨骼验证，CURRENT 也不代表具备运动资格。

## 本地验证和待完成

用户最新边界：血量专项调查和额外采样暂停，已有真实样本保留；不删除字段或链路，不填补假值。关于服务器下发的推测待以后证实。本次有界只读窗口已经结束，仅收尾 LAN、Living 单连接重连、骨骼、View 和时间契约，未接通运动输出。

最新远端短窗：2026-10-12 北京时间 22:50 的新版 Reader 562719C7... / PID 3368、游戏 PID 18952 前后身份核对一致。一次 20 秒 Living/View 直连，第十秒仅 Living 重连；唯一人物与本地控制上下文关联，20 份当前人物观察血量可用 20/20（仅定期快照范围）、真实值 4.802797317504883–74.74000549316406，缺失终态样本 0；每份 20 主干节点、源骨架总数 226，20 份 View Source/Pose 均可用。基线 ACK 各 4，增量 592 / 597，心跳回显 76 / 75；意外断线和诊断错误 0，View 会话不变，客户端已退出。官方 Protobuf 对实际报文的血量/骨骼/相机及固定基线增量核对通过，四锚点净 RTT 复算一致，单程 unknown。单人物短窗不代表所有敌人或整局，World root_unverified/cache_used 仍保留，无串口和鼠标动作。完整新实例产物见 [新版血量只读验收](acceptance/reader-health-562719c7-20261012/REPORT.md)。

离线复核使用 `tools/verify-reader-evidence.py`（官方 Protobuf 独立核对原始报文和血量来源）及 `tools/summarize-reader-observations.py`（汇总期间复制的定期核验状态）。43 份状态中初始等待 3 份、Living/View 当前各 20 份；快照比例不是全部线路帧的字段覆盖率，观察器时间不是源采样时间，失败不填默认值。本轮两个工具对新产物验证通过；原生客户端、协议和上游快照未修改。

准备阶段历史：Reader 新版 SHA256 562719C7...、PID 23084 和原生监听已由保存快照核对，配置游戏 PID 20428。随后上游通知远程重启后 Hyper-V 尚未启用，CR3_CALLER_ROOT caller_cr3=0、采集 opened=0；本项目一次 SSH 原始日志复核超时，没有启动新的 20 秒订阅，也没有血量可用率样本。当时停止重复验收并等待后端恢复、实际新游戏实例及采集就绪通知，见 [新版复验准备记录](acceptance/reader-health-562719c7-preflight-20261012/REPORT.md)。本项目不操作鼠标、串口、后端或启动配置。

最近完成的非空远端验收（历史）：Reader 适配 2609.1.3.5 后，2026-10-12 北京时间 22:11 的 20 秒单实例直连收到 Living 12–14 人，保存样本每人 20 个有效主干节点、源骨架总数 226；View 的位置/旋转/FOV/原点可用。两类各发送 4 次基线 ACK，增量 Living 591 / View 596；第十秒仅 Living 重连，View 保持会话。四时间戳独立复算通过、单程 unknown，意外断线与诊断错误为零。人物血量全部 DependencyBlocked，部分人物状态/装备字段亦缺失；场景未独立核对，不冒称整局或实战控制。完整原始记录、核验脚本及缺口见 [2609 非空接入记录](acceptance/reader-lan-2609-20261012/REPORT.md)。该历史不证明新版血量修复或当前后端已恢复。

Debug/Release 八个项目编译、原有 PC 回归与 reader-tests 通过。Reader 测试含 21 个功能断言：精确基线 ACK、重投保留采样、固定基线增量恢复、跨域/无时钟接收、零 t0、净 RTT、探测失败/超时保留实体、心跳挑战、类别隔离、失效下界及恢复、记录跳号、源内倒序、握手时钟回显和本地提交时钟倒退。

独立回环使用官方 Python protobuf 7.36.2 动态加载冻结 descriptor，解码 C++ 夹具后由官方 runtime 重编码，运行真实只读客户端。五场景：正常 ACK/增量/失效/恢复及 Living 单连接重连、无时钟声明、流量存在但无心跳回显、超限长度、半包停滞。普通场景中 Living 建连两次/ACK 四次/增量两次/失效确认两次，View 建连一次/ACK 一次/增量一次；探测计数受窗口调度影响，以日志为准。

```powershell
./tools/test.ps1 -Configuration Release
# Python 环境需安装 tests/reader-test-requirements.txt；正常客户端不需要 Python。
./tools/test-reader-loopback.ps1 -Configuration Release -Python <具有protobuf的python.exe路径>
```

日志与 JSON 计数在 `out/reader-loopback/<Configuration>/`，本轮验证使用已安装的 `G:\pubg-hyperv-memory-reader\bin\protocol-tools\Scripts\python.exe -B`，未写入上游。

第一次远端现场记录（历史）：2026-10-12 已收到部署/监听证据并执行一个客户端的 20 秒直连。Living 两次握手，第十秒换会话，View 保持原会话；心跳和跨机探测通过、意外断线与诊断错误为零。两类始终 WAITING，基线 ACK/增量均为零：Reader 日志显示目标游戏 27908 映像指纹 ImageMismatch，采集批次未开放。通信验证与数据阻塞分开记录，实际实体/相机数据验收仍待 Reader 恢复兼容后补齐。完整产物见 [20 秒现场记录](acceptance/reader-lan-20261012/REPORT.md)。没有将 Reader 数据接入鼠标运动。
