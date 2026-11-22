# 本机静止断开隔离记录（2026-10-21）

当前新板设备已恢复 **固定生产v6.6**，28180字节全部读回，生产工作树仍v6.6，PC仍为1.2.8。**用户已要求暂停，不自动继续烧录/取证；故障尚未修复，根因未确定。** 用户说的约5秒是体感，不能把它当固件计时规则。蓝色USB始终接当前本机，未切换远端。

## 最新暂停状态

用户已对同v5测试明确反馈“仍需晃几下才恢复”。随后标准核心固定14110字节诊断短测35秒，Release在2626/5270ms，其后约16秒RX不变时稳定；不能把启动事件直接等同长期静止故障或据此排除整个自定义程序。延长计划120秒，第一窗在约29308ms后串口ReadLine取消，未完成，不计通过。取消是否与物理插拔有关未取得答案，用户回复“刚刚没测试，先停一下”，停止继续调查，不推断取消原因或把该轮算人工测试。

两轮均先备份/复核Flash、读EEPROM，再写标准诊断并读回；finally均恢复v6.6 FB4A3D0B427271FB9AFEEF3A16B3A9F28EA8D429CD692E05651C80C2E74E11E3，28180字节全部读回，最后23:04:59。收据`build/standard-host-isolation-20261021/run-result.json`及`extended/run-result.json`。之后45秒CDC与45秒Windows被动记录已结束，暂停时释放句柄，不新启动检查或恢复软件。无fuse/bootloader/EEPROM、远端、Reader/Radar或电源策略修改。标准诊断程序没有留在实机，原始资料全部保留供用户将来允许继续时分析。

## 历史22:49：新板同一v5镜像对照与标准核心准备

22:44用户确认旧板拔掉后，无旧板或编程器遗留。22:46仅对新板USB1一次复位，完整备份/复核Flash、读EEPROM后写固定v5 A144FDBA382D6EDD8B55A8B920B5A2035320419981FC63DBDE9F528C03962B1A；该镜像与旧板本次写前备份的25464应用字节完全一致，新板写后全部读回。原28180字节匹配v6.6，EEPROM与旧板原值一致。没有fuse、bootloader、EEPROM写入，不覆盖旧部署收据。原始证据与汇总见`build/new-board-v5-ab-20261021/`。

30秒记录7016→36017ms出现ready0于17016/34017；后续59079→118285ms鼠标RX8354约59.206秒保持，ready0于59079/98675/99675/100675，xerr1→4。控制accepted/steps/synthetic全0；软件在第一轮后由脚本外重开，不称整窗PC关闭。v5没有Release原因计数，不能推精确释放次数；只可确认同一v5应用在新板仍掉就绪，不是v6独有回归。异步请求用户静置后首次移动反馈尚未收到，当前保留v5等此轮观察，不能声称手感已恢复。

最小标准核心观察程序`firmware/standard_host_probe/`已经通过AVR构建，尚未烧录。`tools/build-standard-host-probe.ps1`从固定SHA归档独立使用官方ArduinoCore-avr1.8.7、未改UHS1.7.0，保持全局平台和生产核心原样。去掉自定义USB核心、GHUB转发、TTL与鼠标透传，默认库调度不作为1000Hz交付。Flash14110/静态SRAM734/余1826不是栈水位；HEX46EB8B0E3E3496369E93665A3068962CBEA1720DF8D332186689F1D3EEB74E00。后续测试会短暂没有透传，先完成当前用户观察并准备恢复，不把“已构建”记成“完全标准核心实机对照已做”。

用户回复“你随便烧录，现在接的是原电脑啊”，已授权本次本机透传修复的固件烧录。该授权不扩展到其他设备、远端、电源策略或自动输入。本轮按各候选备份、单次写入与读回完成，没有反复要求人工RESET。备份目录的source/platform是当时生产工作树，不一定是被备份二进制的对应源码；旧版本对应源码以固定release为准。

## 对照结果

| 固件/隔离项 | 后台记录 | 能支持的结论 |
| --- | --- | --- |
| 与旧板完全相同的v5应用 | 新板25464字节读回；30秒两份ready0，后续约59秒RX8354不变仍出现ready0，控制0 | 异常不限定于v6；手感反馈待收到，不足以确认电气根因 |
| v6.5，延后连接采样 | 30秒：鼠标RX482不变，release1→13/init2→13，控制steps/synthetic0，PC关闭 | 延后采样未解决 |
| v6.6，事件后既有IN的新DATA/NAK证明 | 30秒：release0→7/init1→8；checks0→7、alive/nak0、lost0→7；最新错误0D。窗口混合静止/人工移动，PC在中途由脚本之外重开；最早释放在accepted0时发生 | 事件后没有得到可证明在线的新回应，不能直接忽略SE0。2500µs协作期限不是全部ISR/调度的硬实时保证 |
| v6.7，SPI请求4MHz | 30秒：鼠标RX0不变，release1→12/init2→13，alive/nak0、lost1→12；steps/synthetic0，PC关闭 | 降SPI没有消除当前故障，不能说SPI速率是已确认原因 |
| 临时原始库轮询 | 30秒：直接HIDComposite::Poll/ParseHIDData、不启用本地连接复核，release0→12/init1→13，鼠标RX0，steps/synthetic0 | 自有异步IN和新的连接复核不是必要触发条件；仍保留共享USB核心、控制桥、硬件，不等于完全排除软件或证明硬件损坏 |

临时库轮询按最大端点间隔10ms运行，不是1000Hz正式实现。BUS poll/nak和LOAD xerr只统计自有异步路径，因此此对照中它们为0不能解释成没有USB轮询或没有错误。连接检测未启用，CONNECT0同样不能解释为没有连接事件。原始库、完整PC USB核心、描述符、报告FIFO和4MHz配置均来自固定输入，只有独立构建副本的sketch与版本标记不同。

原始轮询的持续CDC记录结束于板端32889ms；随后关闭CDC。90535ms重新打开的首份摘要显示release21/init22，保留的最后DROP为55436ms，处于关闭CDC期间，控制steps/synthetic仍0。后来出现人工移动数据。由此可排除“必须持续打印诊断才触发”这一必要条件，不能用当前D0/susp0排除所有历史电源变化。没有记录真实手动起动时刻，不能给出首份移动丢失或精确唤醒延迟。

## 历史21:55恢复的设备与源码（22:46新板已改为固定v5对照）

21:55:57完成恢复v6.6：HEX SHA256 `FB4A3D0B427271FB9AFEEF3A16B3A9F28EA8D429CD692E05651C80C2E74E11E3`，28180字节写入及完整读回通过。先备份并复核32KiB Flash、读1KiB EEPROM；备份中的27594字节对照应用逐字节匹配固定测试HEX。没有写fuse、bootloader、EEPROM。恢复收据：[restoration.json](../build/production-restored-20261021/restoration.json)。

4MHz实验的四份生产源码变化已在核对84份输入后恢复为固定v6.6，保留全部实验副本/HEX/日志；没有重置其他用户代码。SPI回到26MHz请求，在16MHz AVR上预期8MHz，未做电气测量。USB物理调度1ms、TTL500000、控制最小2ms/各轴±100/预算200保持；参数与离线事件模型不能代替实际1000Hz/500Hz性能验收。

恢复后的3秒摘要确认版本v6.6、真实人工移动持续收发、ready/known真、steps/synthetic0；这是启动/移动确认，**不是静止修复通过**。Flash28180、静态SRAM2057/剩余503，不是栈水位。PC1.2.8随后按既有授权最小化重开，程序文件不换版。两处全部业务配置、主题语言和各67份模型/采样/报告逐值/逐SHA保持：60Hz、重力5.5、距离600、DPI1600。

恢复后固件全套离线回归及架构336源码再次通过，无遗留AVRDUDE；正式PC main36964/native39604。测试日志与最终核验在`build/production-restored-20261021/`，未进行前台自动操作、Reader/Radar修改、提交或推送。

## 已知边界与下一步依据

已捕获的链路是Host连接事件/锁存SE0 → 库进入DETACHED_INITIALIZE → Release清理 → 重新枚举；v6.6事件后IN给出的是错误0D，未得到新DATA/NAK。MAX3421E寄存器记录不是D+/D−波形，不能把它直接等同于真实拔出。FRAME计数可合并、最近PowerData和原生USB epoch也不提供完整USB总线或供电历史。

下一步应取得能区分共享Host驱动、接收器行为与新底板/Shield电气条件的独立证据，例如不同USB设备的交叉对照，或同步测量接收器端VBUS、3.3V和D+/D−。现有日志不足以认定某个稳压器、引脚、接收器固件或芯片已坏；继续修改延时或强制在线没有证据支持。尚未执行这些物理测试、无自定义USB核心的最小主机测试或栈水位实测。

芯片行为的参考是[MAX3421E编程指南](https://www.analog.com/media/en/technical-documentation/user-guides/max3421e-programming-guide.pdf)与[上游库](https://github.com/felis/USB_Host_Shield_2.0)，它们不证明当前硬件的具体根因。

## 原始证据

- `build/idle-confirmation-20261021/`：v6.5构建、烧录、30秒失败记录。
- `build/post-event-response-20261021/`：v6.6真实回应核验、首次烧录、30秒记录。
- `build/spi-compatibility-20261021/`：4MHz构建、固定输入、烧录、30秒记录、完整业务数据核验。
- `build/stock-poll-isolation-20261021/`：原始轮询副本、构建、固定输入、烧录、30秒与关闭CDC后记录。
- `build/production-restored-20261021/`：生产源码/设备恢复、旧对照应用比对、启动和PC重开记录。
- 各`releases/*/manifest.json`是准备清单，保留deployed=false；实际状态见同目录deployment.json，不改写准备历史。
