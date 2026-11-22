# 2026-10-19 Flash 写入起点故障定位

## 2026-10-21 旧底板新授权的单次尝试

用户在新板修复任务中重新插入旧板并明确授权尝试v6。旧板COM17只读NATIVE v5，物理路由USBROOT(0)#USB(4)#USB(4)；新板COM12/boot COM10路由USBROOT(0)#USB(1)。工具新增运行/boot精确物理路由与实例筛选，模拟通过后等待用户一次旧板RESET，选择旧COM4/2341:0036\6&5E931A0&0&4，未复位或读写新板。

22:35:53原版AVRDUDE单进程先读32768 Flash、1024 EEPROM、复核32768 Flash成功，再仅一次写固定原始v6应用8E50F113409A25B6FAAC5F24CFF56284D7171DAEC7DBD373AF37D1C8486CBDD2。仍停Writing25514，22:36:38到45秒期限请求终止25120，无页完成/成功读回，不自动重试。写前旧应用25464字节与固定v5完全匹配，完整备份Flash SHA FDA1A91D14985A69BA2D4C2A7414B81F2823F3B919340D5D130D8D480C368A4C、EEPROM原值；写后完整性尚未核验。没有fuse/bootloader/EEPROM写入。

证据build/old-board-v6-attempt-20261021和backups/backup-old-v5-before-v6-20261021-223531。25120仍卡驱动时已请用户只拔旧板USB清理，非准备再烧录；其后已确认编程器退出、最后旧COM4仍枚举，等待用户清理/后续只读结果。PC1.2.8已最小化重开、数据不变，新板未复位/编程。本节不能把旧板升级失败等同于新板Host静止断开根因。旧板正常是用户报告；新旧当前固件不同，须保留这一对照边界。

适用设备：本机 FUGUI 上的 Leonardo/ATmega32U4 底板，原生 COM15，复位后 COM10。Host Shield、接收器、TTL 和杜邦线断开。当前固定应用仍为 native-v5-speed；PC1.2.3/native-v6 没有成功部署。

## 结论与证据边界

现有证据将故障集中到 **Caterina 的 Flash 擦除/编程分支没有完成应答**。不能再将它泛称为“500000 波特率不支持”“固件太大”“末页没补齐”或“首地址代码有问题”。尚未直接读取真实 CPU 程序计数器/SPMCSR，也没有本次失败的总线抓包，因此**不能把静态代码地址当成已观测的硬件断点，不能直接宣布芯片损坏**。

Caterina 在收到 B 命令头后关闭 TIMER1 超时计时，然后先做页擦除并等待 SPM 完成，之后才接收数据、填充临时缓冲和提交页面。真实设备备份的反汇编中，擦除 SPM 位于 0x749C，等待位于 0x749E–0x74A2，提交位于 0x756A；这些是静态控制流位置。擦写分支若不完成，既没有 CR 应答，也无法靠正常 8 秒超时返回应用。完整掉电同时复位 MCU 和 Windows USB 设备状态，解释此前为何需要拔线恢复；底板 RESET 也是 MCU 复位手段，不把断电称为唯一方案。

## 本轮采用的新探测

没有重新上传整份 v5/v6。所有源文件、测试和工具均在本仓库 firmware 目录。

| 探测 | 实际结果 | 能说明什么 |
| --- | --- | --- |
| 15/16/17/32/128 字节无存储作用的同步字节载荷 | 全部完成并能继续识别 CATERIN | 通用发送长度/16 字节边界不是复现故障的充分条件 |
| g 读取命令头分两次发送 | 128 字节内容与旧备份一致 | 等待跨包参数、NUL/0x80 参数及 Flash 读取路径正常 |
| B 00 00 E | CR 应答，EEPROM 前后一致 | B 解析与零长度非 Flash 分支正常；此命令不进入擦除/提交 |
| T FF | CR 应答 | 高位数据字节可正常送入无存储作用的命令 |
| C FF 临时缓冲装载 | CR 应答，之后用 g 清除缓冲 | 一般 SPM 页装载可返回；没有执行 m 或页擦除 |
| 0x6F80 空白页原样擦写一次 | 6 字节 Send 完成，未收到页完成 CR | 故障也发生在无应用代码的空白页，且没有应用层分段 |

空白页操作：先核验固定备份和实际设备的 0x6F80–0x6FFF 全为 FF，再显式重设地址。仅发送一次 `42 00 02 46 FF FF`（B、长度 2、F、FF、FF），在已有 FF 临时缓冲上保持整页原值。命令和数据共 6 字节，小于 Caterina 16 字节端点大小。PC Send 在 **2.0612 ms** 返回成功，随后接收页确认超时，设备仍停留 COM10。没有触及 v5 代码、0x7000 起的 bootloader、EEPROM 或 fuse；这项实际页擦写须与其余不改变持久存储的探测明确区分。

PC API 的 Send 成功不是总线抓包，不能单独证明 CPU 运行到了某条指令。结合 B 非 Flash 分支、SPM 临时缓冲和历史首块载荷失败后的应用完整性，**最值得继续核验的是页擦除动作及其完成等待**；仍保留提交/USB 特定交互等未直接观测的可能性。尚缺稳定供电电压测量、ISP 独立编程通路对照或真实 CPU 调试，来区分供电、Flash 擦写硬件和启动加载器执行状态。

## 状态核验与工具修正

用户完整掉电后，22:43:09 仅执行 VerifyOnly：完整读取 28672 字节应用，差异 0，SHA256 `62307E81F163402FD8E50E6C9B83315122C8C563D535C149EC3CBA3878947D77`；EEPROM 1024 字节 SHA256 `5F4ECDB7B71C3E403983FE405CDDCDC2F2576B655FDB3E80D94A6F7C32E58BC2`，均与旧备份一致。fuse FF/D8/CB、lock FF。该核验发生在最后一次空白页故障之后。

发现并修正恢复传输中的另一个问题：原先取消 I/O 后调用 `GetOverlappedResult(..., true)`，在 USB 驱动不完成取消时可能无限等待。现在取消后最多额外等待 250 ms；未完成则退役该隔离传输，将仍被驱动引用的缓冲/事件/句柄保留到进程清理，避免提前释放。探测入口另用 20 秒父进程期限，超时只请求终止自己的子进程，不无限等候退出。它消除了该处软件无限等待，**不等于修复板端擦写或 Windows 内核挂起**；真实不可取消驱动分支未通过再次擦写来复验。正常读取及 PS5/PS7 协议与错误拒绝检查通过。

系统只读取证：原生与 bootloader 均使用 usbser，串口设备/Ports 类未见额外上下过滤器；USB 类存在已安装的 USBPcap 1.5.4。没有移除过滤器、换驱动或安装程序，也不把其存在当成已确定根因。历史七份完整 Flash 备份的 bootloader 4KB SHA 一致，且匹配本机官方 Caterina；本轮未重写它。

## 23:29 用户要求的最后一次完整上传

用户明确授权最后再试一次v6。上传前23:28:17完整只读28KiB应用及1KiB EEPROM仍与旧v5备份一致。随后仅执行一次固定v6应用上传，1200复位与唯一COM10捕获正常，原版AVRDUDE8仍停在写入0%，45秒后请求终止80572；没有成功页确认或v6读回。外围接线沿用上一轮已拆下的记录，未取得本轮新人工确认，因此不将本次说成重新确认外围断开的独立对照。

已停止进一步写入。终止中的80572一度仍可按名称枚举，23:30:35已确认AVRDUDE退出，底板仍COM10。待完整掉电恢复和只读核验；不能把上传前旧程序完整性当成本次失败后完整性。证据`build/final-upload-20261019-232703/`，正式PC1.2.2关闭，PC1.2.3/native-v6均未部署。后续仅恢复与核验，不重复上传。

23:35收尾只读复核：实际原生COM12重新出现，AVRDUDE已退出。VerifyOnly仅执行身份校验和1200触发，8秒未发现bootloader后退出，没有发送Flash/EEPROM存储命令；本次失败后的完整性仍未知。没有第二次写入，没有继续要求RESET/拔插。旧PC1.2.2已最小化恢复，nativeCapture.active=false；新版PC和v6均未部署。追加证据final-upload-20261019-232703/postwrite-verify.log、restored-startup.json和final-cleanup.json。

## 2026-10-20 用户重新授权的单次完整上传

本轮为用户在新一轮明确提出的最后一次尝试，没有自动复用上次授权发起循环。原生COM12，软件1200触发未进入bootloader，未发存储命令；用户按提示人工RESET后14:48:09捕获唯一COM4/2341:0036。原版AVRDUDE8单进程顺序执行：读取Flash32768和EEPROM1024、分别对固定旧备份验证、写固定v6应用并启用默认读回。两个前置验证均通过，实际导出SHA也与固定旧备份完全一致。

写入仍停在首次Writing 25600 bytes to flash，无页完成/成功v6读回。14:48:54达到45秒期限，已请求终止26300，未再上传；启动文件和result.json限制唯一编程器调用。候选和协议不改，没有写bootloader、EEPROM或fuse。超时后进程一度仍可枚举、板停COM4，用户被要求仅拔蓝色USB以清理挂起，不能将上传前的旧数据完整性当作失败后完整性。PC1.2.3/v6未部署，本轮开始时旧PC未运行。

14:50收尾确认编程器26300与捕获23740均已退出，14:50:32实际设备列表为空；用户明确回复已拔蓝色USB。没有再次上传、复位或存储读取，失败后完整性仍未知。最终清理证据final-cleanup.json。

证据`build/final-upload-20261020-1448/`：prewrite-flash.bin、prewrite-eeprom.bin、avrdude.stderr.log、capture.stdout.log、result.json。该对照再次确认读/验证路径正常而写入路径不完成，仍不足以单独认定芯片物理损坏。

## 文件

- `build/write-stall-audit-20261019/read-probe.log`：22:34 七项无持久存储改变的探测及应用/EEPROM核验。
- `build/write-stall-audit-20261019/buffer-probe.log`：22:37 临时页缓冲探测。
- `build/write-stall-audit-20261019/blank-page-probe.log`：22:39 唯一空白页尝试；失败，不是部署成功证据。
- `build/write-stall-audit-20261019/post-probe-verify.log`：22:43 最后失败之后的应用/EEPROM完整核验。
- `tools/avr109-read-probe.ps1`、`tests/avr109-read-probe-tests.ps1`：独立探测与模拟拒绝测试。默认不执行真实 Flash 擦写；BlankPage 不能用作自动升级重试或降级路径。

## 原始依据

- [Arduino Caterina 源码](https://github.com/arduino/ArduinoCore-avr/blob/1.8.7/bootloaders/caterina/Caterina.c)：ReadWriteMemoryBlock、FetchNextCommandByte、T/C/B 命令与 TIMER1 管理。
- [Microchip ATmega16U4/32U4 数据手册，27.7.2–27.7.3](https://ww1.microchip.com/downloads/en/devicedoc/atmel-7766-8-bit-avr-atmega16u4-32u4_datasheet.pdf)：页缓冲装载、RWWSRE 清除、页擦除/提交。
- [AVRDUDE v8.0 butterfly.c](https://github.com/avrdudes/avrdude/blob/v8.0/src/butterfly.c)：正常写页以 B 头和数据等待 CR。

没有通过变更 Reader 字段、远端部署、自动游戏输入或重编旧固件来参与这次定位。
