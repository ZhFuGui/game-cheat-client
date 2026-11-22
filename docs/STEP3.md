# 第三步：本机串口控制鼠标盒子

版本：PC 0.1.0 / 固件 native-v3-control，2026-10-12。

标准 VS 解决方案、单向串口控制、小幅位移、松键停止已经实现。Debug、Release、固件回归和描述符检查通过；新固件已烧录并读回校验。本机及远程桌面的 F8 小幅右移、松键和长按上限已验收。最终接线为红色模块接本机 FUGUI / COM14、蓝色原生 USB 接远程 DESKTOP-J3T11UM / COM3；端口以每次枚举为准。扩展功能和长期表现仍待验证。

## 使用 Visual Studio

1. 打开根目录 `pubg-mousebox-aimbot.sln`，选择 `Debug / x64` 或 `Release / x64`。
2. 将 `mousebox-controller` 设为启动项目。首次可在“属性 → 调试 → 命令参数”填写 `--list`，查看串口。
3. 确认红色 USB-TTL 已出现后，清空命令参数并运行；唯一受支持的模块会自动选择。多个模块时填 `--port COMn`，使用实际端口。
4. 先轻轻移动一次实体鼠标，再按住本机 F8。默认向右每 10ms 移动 1 个鼠标计数，每次按住最多 32 个计数、最长 320ms。
5. 松开 F8 停止程序修正。一直按住达到上限后不会重新触发，必须松开再按。Escape 或 Ctrl+C 退出。

位移单位是鼠标计数，不是桌面像素；系统鼠标速度和加速会改变可见距离。启动时已经按住 F8 不会触发，需先松开。

命令行入口也可使用：

```powershell
& .\tools\test.ps1 -Configuration Debug
& .\tools\test.ps1 -Configuration Release
& .\out\x64\Release\mousebox-controller.exe --list
& .\out\x64\Release\mousebox-controller.exe --probe
& .\out\x64\Release\mousebox-controller.exe --key F8 --dx 1 --dy 0 --interval 10 --limit 32
```

## 接线和设备识别

| 接口 | 接到哪里 |
| --- | --- |
| 红色 USB-TTL 的大 USB 插头 | 本机 USB 口 |
| 模块 TXD（照片中的绿色线） | Leonardo / Shield 的 0/RX |
| 模块 GND（照片中的橙色线） | 板子的 GND |
| 模块 RXD（照片中的黄色线） | 本阶段悬空；未完成电平核验 |
| 模块 +5V 和 3.3V | 不连接板子 |
| Logitech 接收器 | Host Shield USB-A |
| 板子原生 USB 蓝色线 | 接收鼠标输入的电脑；开发烧录时可先接本机，最终接远程主机 |

固件使用 Serial1 的 0/RX 接收，115200、8N1，禁用 UART 发送脚。本机打开的应是 USB-TTL 端口；板子原生 CDC（本机此前为 COM12）只用于诊断和烧录，控制程序会拒绝把它当作 TTL。

### 切换输出电脑时的已知限制

红色模块保持供电时拔蓝色 USB，用户实测底板微亮；拔掉绿色 TXD 灯灭，存在经信号线残留供电的现象。切换后可能鼠标失效，完整断开后可恢复。用户接受暂时通过拔插红色模块处理，本阶段尚未消除这一问题。

切换时先退出控制程序、拔掉红色模块 USB，保持 TXD/GND 的正确接线；拔蓝色线，确认底板灯灭后再等 5 秒。先把蓝色线接目标电脑并确认物理鼠标正常，再把红色模块 USB 接本机，然后运行控制程序。本次按此顺序恢复远程输入并通过 F8 验收。若无法恢复，保留失败状态用于诊断；这个操作顺序不等于已经保证任意热插拔。

红色模块 USB 身份为 `10C4:EA60`，描述为 CP2102。2026-10-12 已安装微软目录提供、微软签名校验有效的 Silicon Labs 驱动 11.5.0.417，发布名 `oem262.inf`。用户发现此前 USB 插入不够深，插稳后识别为 **COM14**，合法帧接收已通过。端口可能变更，每次按当前枚举为准。指示灯亮只能说明有供电；Leonardo RX/TX 指示灯显示原生 USB 通信，不用于判断 0/1 引脚串口是否收到数据。

驱动来源：[Silicon Labs](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers)、[微软目录匹配条目](https://www.catalog.update.microsoft.com/ScopedViewInline.aspx?updateid=32293688-4758-4334-98c0-7d634cf3c59e)。官方直链本轮返回拒绝访问，改用微软目录的匹配驱动。安装日志和下载包保存在固件 `build/drivers/cp210x/`；这些本机文件不纳入 Git。`mousebox-firmware/tools/install-cp210x.ps1` 验证目录签名和硬件身份后安装，Windows 要求管理员权限。

## 停止规则

本机每约 2ms检查按键，每 10ms刷新运动状态。独立线程负责串口写入，控制线程不等待一次串口写入；发送队列只保留最新运动，STOP 优先清掉待发运动和该会话尚未发送的 START。正在写入的完整帧先完成，再发送 STOP。

固件每轴单步限定 -4..4，步间隔 5..50ms；每个 START 的总修正上限为 64 个计数，默认 32，按绝对 X 与绝对 Y 的和扣减。刷新只能更新有限速度和期限，不增加总计数。序号严格递增，重复运动不能续期；停止后的当前会话不能重新激活，需要新会话。

本机发送的运动许可为 80ms；新有效运动指令未到达时，固件在正常主循环调度下撤销运动。进程退出会发送全局 STOP；异常终止、拔线或无法写入时依靠期限和总计数上限。80ms 从板端最后接收有效运动指令起算，不能当作任意驱动阻塞、USB 故障或同步恢复下的端到端停止保证。

松键停止的是程序修正。实体鼠标继续透传。端点忙时重新合成未提交报文，只在 USB 接受后扣减修正；STOP 保留物理 FIFO，撤销尚未提交的修正，无法撤回已交给 USB 的报文。

鼠标静止时使用最后已提交的按钮状态和五个厂商字节，X/Y 为本次修正，两个滚轮归零。首次收到物理报文前不生成初始状态。厂商字节缓存方案只针对当前接收器，仍需 G HUB 实机回归；软件测试仅确认字节保留和滚轮不重放。

## 实机验收顺序

1. `--list` 确认 CP2102 的实际端口；连接不稳定或无端口时先处理 USB 和驱动。
2. 打开盒子的原生 CDC 诊断，再运行 `--probe`。它只发送探测和停止，观察 `CONTROL v3` 的 accepted/stops 增长；串口写入完成不等于盒子已接收，单向 TTL 没有回执。
3. 移动实体鼠标一次，运行默认控制程序，在普通桌面短按 F8，观察小幅右移；松开后保持鼠标静止，确认修正停止。
4. 按住到自动上限、松开再按、切换轴方向；按住时关闭程序或拔掉 TTL，观察停止。
5. 将板子原生 USB 接远程主机，红色模块继续接本机，重复上面的桌面验收。本机和远程主机的端口分别发现，不照抄 COM12。
6. 回归物理移动、按键、滚轮、G HUB。新版本前台回报率、停止延迟、长期稳定性和栈水位另行测量。

固件构建使用 `mousebox-firmware/tools/build.ps1`，PC 构建不会隐式烧录。

## 当前验证范围

- PC：Debug/Release 的标准 VS 项目通过（原六个；移除 reader-bridge 后为五个）；按键边沿、启动已按住、上限后重新触发、STOP 队列优先、迟到旧运动、CRC、缺字节及错位恢复通过。
- 固件：既有透传/HID++ 状态机回归和新增控制测试通过，覆盖期限、当前会话重复、micros 回绕、有限预算、端点忙重试、XY 边界、静止按钮、滚轮归零、STOP 保留物理队列及运动刷新洪泛。
- 设备：AVRDUDE 应用区写入和 24584 字节读回通过；启动诊断 ready=1、active=0、rx_only=1，HID++ 枚举流量存在，当前检查无传输/队列错误。
- 静态资源：Flash 24584 / 28672 字节，SRAM 1955 / 2560 字节，静态剩余 605 字节；尚未测量最深栈水位。
- 独立 TTL：COM14 无运动探测后 COM12 诊断 accepted=3、stops=2、steps=0、crc=0，日志 `mousebox-firmware/logs/native-v3-control-probe-received-20261012.log`。
- 本机 F8：用户确认向右小幅移动、松开停止、长按到上限停止。日志 `mousebox-firmware/logs/native-v3-control-f8-local-20261012.log` 累计 steps=245、synthetic=245，结束后 active=0 且修正不再增长；rejected/crc/overflow/invalid/xerr=0。程序已用 Esc 退出；约 1 秒诊断周期不能证明毫秒级停止延迟。
- 双机最终接线实测：红色 USB 接本机 FUGUI / COM14，绿色 TXD→0/RX、橙色 GND→GND、黄色悬空，蓝色原生 USB 接远程 DESKTOP-J3T11UM / COM3。完整掉电后的远程日志 `mousebox-firmware/logs/native-v3-remote-after-full-power-20261012.log` ready=1、物理 RX/submit 增长；跨机无运动探测 accepted=3、stops=2、steps=0、crc=0，日志 `mousebox-firmware/logs/native-v3-control-probe-remote-success-20261012.log`。
- 远程 F8 验收通过：用户确认“远程小幅右移，松开停止，长按到上限停止”。60 秒日志 `mousebox-firmware/logs/native-v3-control-f8-remote-20261012.log` 最后完整控制行 accepted=126、stops=12、steps=159、synthetic=159；随后 23 条控制行保持不变，active=0，rejected/crc/overflow/invalid/xerr/cerr=0。程序已退出、远程诊断已关闭。计数为累计值，不是单次按键位移，秒级诊断未测毫秒级停止延迟。
- 待验收：轴方向/异常停止扩展、合成报文厂商字段的完整行为、G HUB 功能以及新版本长期表现。切换 USB 的供电/恢复问题保留为上述使用限制。

本项目已移除 reader-bridge 工程；后续由本机直接订阅远端 Reader 的 Living/View。Windows 时钟工具保留，接口与解析准备见 [Reader 接入边界](READER_INTEGRATION.md)。Reader 消费、校准及数据驱动控制尚未实现，本次 F8 测试位移不使用 Reader 数据。
