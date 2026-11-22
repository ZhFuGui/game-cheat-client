# 静止时物理透传异常：当前恢复v6.6，根因未定

2026-10-21。用户已授权本次本机修复烧录，蓝色USB始终本机。v6.5/v6.6仍复现，4MHz SPI和临时原始库轮询对照均未消除断开，CDC关闭期间也有释放。21:55:57已恢复固定v6.6、原SPI和完整异步透传，28180字节读回，PC仍1.2.8。当前未修复、未证明电气根因。新增对照/精确范围/恢复证据见[HOST_ISOLATION_20261021](HOST_ISOLATION_20261021.md)。下文保留历史范围，不把候选未烧录当当前状态。

## 历史：v6.4固定接线结果

trial2后台60秒有256条Raw Input/583份电源光标，五节点最近状态D0，用户反馈“难说”，首份响应不作通过/失败。窗口结束后另开CDC只读8秒，RX三端点不变、NAK增长，release273→276/init267→270，异常循环间隔2617/2614/2611ms，task11/SE0，steps/synthetic0。已独立复现v6.4 Host释放，不是将手感不确定倒推为失败。用户约5秒只是体感，不声称有定时器；循环周期不是初次静止门槛。最初SE0原因仍未确定。

v6.5候选改仅运行期的非阻塞复核，100µs后采样、200µs总检查期限，不覆盖待采样HCTL，真实断开仍走库处理。固定v6.4/新驱动同一模拟旧失败新通过及原回归/描述符/AVR/架构336通过，未烧录。设计、边界和证据见[IDLE_CONFIRMATION](IDLE_CONFIRMATION.md)。下方14秒/首轮说明仍为历史，不能用于声称改善。

## 历史用户澄清

用户描述的是平时静止约5秒后需动一下才有反应，不是对下面14秒窗口的确认；随后补充记录时在插拔接线。该轮不能作固定接线对照，本窗口已撤回“改善迹象”，不把用户描述与日志逐帧对应。原始记录保留，当前根因和v6.4效果未定。

持续打开复合USB的CDC诊断接口可能改变选择性挂起条件，1Hz的susp0也不是挂起历史。先准备不打开CDC的Raw Input/Windows设备PowerData/光标同步记录；PowerData是Windows报告的最近电源状态，不是USB总线测量。参考[HID电源管理](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/power-management)与[PowerData](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-powerdata)。不改系统电源设置，不启动UAC，不通过假输入维持在线。

## 存在用户接线操作的原始本机记录（非受控结果）

首次12份摘要在PC关闭后采集，Host初始化成功、rev13、额外64次重复读取bad_reads0、state90、ready/known保持；有持续鼠标活动。第二窗40份45444→84447ms，至少14.001秒鼠标RX不变后恢复增长，ready/known始终真、release0/init1/recovery0、xerr/cerr0，FRAME观测43661→81228、最大观测gap2520us。期间没有之前的SE0/重新枚举。已请用户静置约15秒再轻移，物理恢复手感待反馈；短窗未复现不是彻底修复或唯一根因证明，USB接入主机也从远端变成本机。

第二窗中PC在本脚本之外重开，新main31204/native38256，accepted4/stops2（STOP/PROBE）、steps/synthetic/active全程0。不能把40秒全部算作PC关闭测试。原始runtime-v6_4-local.log、idle-v6_4-local.log和local-observation.json在build/host-compatibility-20261021，结果已追加v6.4 deployment.json。一次烧录27830字节2.11秒并全部读回，备份为backup-native-v6_3-before-health-20261021-205537；没有自动鼠标位移或按键。

## v6.3和新底板观察

用户恢复原游戏接线并确认物理正常，远端只读10秒仍release3→5/init3→5；DROP task11/bus0/modeD1/hrsl04或0D，曾flags3。RX258→874后多秒保持，accepted/stops/steps/synthetic从启动均0，HOST recovery/irqfb/pending0。CONNECT checks4/incomplete4/fs_again0/hctl04：按当前完成位守卫所有检查均回退，没有成功重新采样的证据。不得以离线模拟通过宣称已修复，也不直接反转完成位解释。数据和自动摘要见根out/reader-sdk71-20261021/box-v6_3-idle-recheck-remote.log及box-v6_3-idle-result.json，实际部署见v6.3 deployment.json。

epoch2/cfg1保持只能说明未观测到原生USB reset；代码中suspend不增加epoch，因此不能据此排除挂起。此前FRAMEIRQ是锁存位，未持续清除计数，看到它为1也不能证明每毫秒仍产生新SOF。

用户拒绝重复TTL接线隔离，该试验未执行且不再待办。新增用户报告：新底板仅重接蓝色USB不一定恢复，需重插接收器；旧板没有这一要求。Host与旧底板来自同商家，新底板来自另一商家。应核查初始化、复位、SPI及持续帧维护，尚不能归因到某个电源/引脚或断言板子损坏。原v5与原v6透传文件逐字节相同，降版不是已有证据支持的修复。

后续候选仅在具备明确更改/取证范围、离线验证和用户新授权后烧录；不伪造输入/在线状态或掩盖真实断开。

## v6.4烧录前的设计与验证记录（历史）

下文为已完成的烧录前验证范围；实际已部署状态和本机结果以上方最新记录及deployment.json为准。详见[固定版本说明](../releases/native-v6_4-host-health-20261021/RELEASE.md)及manifest.json，70个源码/平台输入固定，HEX64EB7FEB021BEF78E342AB115904089C1462D5EEDB2E4A1715343D2D2862A530，Flash27830/SRAM2045/剩余515，不是栈水位实测。

新诊断覆盖首次初始化/68次SPI只读、当前枚举状态和历史错误、原生挂起、掉线判定前寄存器以及真实更新的FRAMEIRQ观察。运行期帧标志清除复用已有传输完成ACK，不增加SPI事务，保留其他IRQ位；枚举WAIT_SOF保持原逻辑。此计数可合并，只是持续活动证据下界，不能等同于USB帧率或鼠标回报率。原SAMPLEBUS守卫、SPI时钟、初始化/复位和恢复策略不变。

测试覆盖30000正常完成/NAK时零新增SPI事务和其他IRQ归属、计时回绕/重新连接、异常/未知修订号只读检查、真实库断开/重采样回归及既有控制/反馈/混合模型。HostTransport逐token核验仅增加观测/清FRAMEIRQ位/重置观测；库只读getter之外枚举逻辑逐token相同，11个保护输入SHA相同。描述符、AVR和架构336源码通过。没有新烧录、复位或自动输入，不将候选当成已完成修复。

## v6.1/v6.2实机结果

v6.1恢复线路后的30秒记录1460有效反馈、0坏帧/丢弃，flags12/13/15分别361/278/821、最大间隔312ms。窗口含按钮活动，不称连续静止30秒；随后CDC有多个鼠标RX不增长窗口，仍ready/known清除、HOST irqfb/pending始终0、native epoch2/cfg1保持。PC30份状态为正常10/接收器未就绪6/按钮未知13/超时1，500000已确认。因此完成IRQ/忙超时不是此复现的已证实触发条件；既有v6.1改动没有解决它。证据box-v6_1-idle-result.json，成功烧录收据已追加失败复验。

v6.2只新增释放前证据和分类计数：`LINK release/init/error/recovery/epoch_clear`及`DROP at_ms/task/bus/irq/hrsl/mode/flags`。释放前3次只读SPI，正常报文路径无额外轮询/写寄存器，不改恢复或控制策略。初始化回调错误单独记录；库若在回调前失败，需结合DROP task判断，error0本身不能证明库全部初始化成功。CONDETIRQ可能已被库清除，因此DROP不是完整总线抓包。

恢复远端后的实际事件：task0x11（DETACHED_INITIALIZE）、bus0（SE0）、modeD1、hrsl04或0D；release后init递增、回调error0、transport recovery/irqfb/pending0。一次DROP flags3证明原ready和known均真，随后被释放路径清理。按本仓库USB_Host库，SE0触发USB::Task的Release再枚举。根据[芯片指南](https://www.analog.com/media/en/technical-documentation/user-guides/max3421e-programming-guide.pdf)第22/52页，连接事件会更新J/K状态，亦可SAMPLEBUS重新采样；现有记录还没有新鲜重采样或电气测量，不能直接认定物理拔出、供电不稳或单一软件缺陷。

正常关闭PC程序后只读CDC10秒仍release41→45，断开at_ms147356/149975/152584/155198，约2.6秒一轮；鼠标RX2355/moving2353全窗保持，控制accepted6/stops3/steps0/synthetic0保持。USB epoch2/cfg1稳定，Host恢复0。用户同时明确补充：未启动整个aimbot时也感觉静止后物理鼠标需晃几下才能恢复。PC1.2.8已最小化重开；Reader/PC自瞄计算不是必要触发条件，真实物理透传受影响。

当时曾提议TTL隔离，用户随后明确拒绝，未执行、不再等待。未忽略断开状态或伪造按钮/鼠标报告。原始证据box-v6_2-idle-events-remote.log、box-v6_2-no-pc-remote.log位于根out/reader-sdk71-20261021。

v6.2构建26380字节/静态SRAM2005/剩余555，原四类及新增证据回归、描述符、AVR构建通过；不是实机1000Hz/500Hz或栈水位测量。20:18用户授权后一次1200复位及一次写入/完整读回，备份backups/backup-native-v6_1-before-link-20261021-201823。完整成功收据见releases/native-v6_2-link-diagnostics-20261021/deployment.json；准备manifest保持原样。

## v6与v6.1早期排查记录（保留当时范围）

用户可复现静止数秒变黄、移动后恢复。第一次12秒/600有效帧记录期间用户一直在游戏中移动，不能作为静止验证。随后30秒记录包含用户主动重复静止/晃动的若干循环，不称作连续静止30秒：1388份CRC有效反馈、0坏帧、最大接收间隔330ms；flags在12（未就绪/未知按钮）、13（就绪/未知按钮）与15（就绪/已知按钮）间切换。约0.6秒恢复就绪后，未知按钮会一直等到新的物理鼠标报告才恢复。

新板只读CDC诊断在t_ms1796201→1797200记录ready1→0、xerr38→39，cerr维持0。结合每次清理后释放代次推进，定位到接收器/Host链路恢复及按钮状态清理；PC显示降级有实际固件依据。不能据此断言新板损坏，也不能把它等同于旧板的Flash写入故障。

v6的HostTransport在超过10ms未看到HXFRDNIRQ时直接请求完整Host恢复；主循环随后清FIFO/按钮资格，初始化MAX3421E并重新枚举。源码和现场现象吻合，但原诊断没有HRSL与具体恢复计数，尚不能区分“完成但漏中断”与“控制器确实仍忙”等根触发条件。

v6.1候选在原10ms期限处追加一次HRSL状态核验。依据[MAX3421E官方编程指南，第10/36页](https://www.analog.com/media/en/technical-documentation/user-guides/max3421e-programming-guide.pdf)，结果为hrBUSY表示传输仍在进行，非BUSY允许按结果寄存器确认完成。已完成的NAK按正常空闲处理；真正BUSY超时仍执行原恢复。正常完成路径不增加查询/等待，不改1ms调度、物理FIFO、HID描述符、500000波特率、500Hz控制、±100计数或已有许可。

新增HOST诊断行分别提供完成IRQ回退次数、真正pending超时次数、最后超时HRSL/端点、原生USB epoch/config及按钮known。它用于下一轮实机确认触发类型，不以离线模拟冒充已找到唯一硬件根因。

离线回归覆盖30000次空闲NAK、定期丢完成IRQ、收到真实报告时只提交一次、真正BUSY继续恢复；原控制/反馈/1000Hz物理与500Hz控制事件模型、冻结描述符和AVR构建通过。Flash25858字节、静态SRAM1986字节，静态剩余574不是栈水位实测。固定版本releases/native-v6_1-idle-completion-20261021，manifest保留准备时deployed=false，实际成功另记deployment.json；真实空闲稳定性、物理1000Hz、控制500Hz和游戏效果仍待接线后验收。

烧录前完整备份backups/backup-native-v6-before-idle-20261021-200640，其中旧应用25514字节逐字节匹配原固定v6。一次1200软件复位、一次AVRDUDE，无人工RESET或fuse/bootloader/EEPROM写入。启动7秒只读看到NATIVE/HOST v6.1、cfg1、ready0/known0、错误/活动/控制0；此时TTL拔下、接收器未就绪，没有进行自动鼠标位移。用户已被请求恢复正常游戏线路，后续需用有效反馈与HOST计数核验真实静止循环；ready0窗口本身不提供空闲NAK修复证据。

正式PC1.2.8已换版，另修“先取比较时钟、后取异步回传快照”的竞态，改为先取快照再取时钟，避免刚到达的报告被判为来自未来。150ms有效期不放宽。悬停信息分开说明回传超时、接收器未就绪、按钮未知和应用资格暂不可用；可选--connection-report只记录30份连接摘要，不开设备、不授予输出资格。当前安装启动Reader两流各60有效帧/s，用户最新设置和全部模型/采样保持；最小化且TTL未接，未验证实际游戏输出。

现场证据位于根out/reader-sdk71-20261021的box-stationary-feedback.json、box-stationary-summary.json和box-after-stationary-remote.log。固件源码前备份、回归及构建日志在build/idle-watchdog-20261021；原成功v6 HEX及新板原始Flash/EEPROM备份保持不变。
