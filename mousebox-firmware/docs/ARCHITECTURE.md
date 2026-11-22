# 固件架构（v7 / 2026-10-22）

固件以“目标板装配”选择硬件，PC 继续只依赖线协议及已明确映射的设备能力。正式已实现目标只有 `leonardo_max3421e_c54d`；`simulated_c54d` 是使用相同核心和应用调度的离线实现，不能烧录。

```text
mousebox-firmware/
├─ apps/relay/                     共享服务调度；Arduino 入口仅 setup/loop
├─ core/
│  ├─ contracts/                   时间、通道、设备端口约定；规范化类型/能力
│  ├─ control/                     会话、CRC 接入、租约/预算、侧键与反馈
│  ├─ relay/                       定长 FIFO、物理与控制报文合成/提交
│  └─ diagnostics/                 空策略；计数策略只在离线测试装配
├─ protocol/include/               PC/固件共用的 v1 帧定义
├─ profiles/logitech_c54d/          接收器身份、报文、端点、描述符、HID++ 请求范围
├─ platforms/
│  ├─ mcu/avr32u4/                 时钟与非阻塞 UART
│  ├─ usb/max3421e_host/            Host 传输状态机与 UHS 适配
│  ├─ usb/avr_device/              原生 USB 注册/提交及 EP0 适配
│  └─ boards/leonardo_r3_max3421e/  引脚/资源与 Arduino 板定义
├─ targets/
│  ├─ leonardo_max3421e_c54d/       target.json、能力导出、实例装配
│  └─ simulated_c54d/              假时钟/Host/设备/串口与离线回归
├─ third_party/
│  ├─ dependencies.lock.json       上游与覆盖文件的逐文件 SHA256
│  ├─ upstream/                    固定 Arduino AVR 1.8.7 / UHS 1.7.0 原件
│  └─ patches/avr_bridge/          4 个必要 USB 核心覆盖文件
├─ tools/
│  ├─ build.ps1 / device.ps1        显式目标构建；显式端口/物理位置/产物烧录
│  ├─ common/ / builders/          目标、HEX、备份读回校验与构建后端
│  ├─ diagnostics/                 单独按需执行的观测工具，不编进固件
│  ├─ recovery/                    低层 AVR109 工具/离线测试，不自动恢复
│  └─ support/                     驱动安装工具，不自动执行
├─ tests/                          核心、硬件适配假对象、混合负载和工具回归
├─ releases/                       固定版本与旧工作树历史归档
├─ backups/                        每次设备操作的 Flash/EEPROM 与收据
└─ build/                          可重建暂存/产物/测试结果
```

依赖方向：`protocol ← core ← apps`。`profiles` 只能使用协议与核心契约；`platforms` 实现端口，不拥有控制算法。最终 `targets` 同时装配应用、profile 与平台。核心可独立用 C++11 编译，无 Arduino/AVR、桌面、动态分配或虚函数端口依赖。静态模板避免在 32U4 上为了分层增加逐包虚调用和堆分配。

Host、队列、会话、UART 和设备窗口均由同一固件循环唯一拥有；ISR 仍由 AVR 核心管理。已收到的物理报文按端点 FIFO 保存，设备忙时保留原包；只有完整报文已进入 USB 发送银行才出队、扣除控制预算。这个“提交”不表示主机已经 ACK。积压不追赶突发，控制超时不重放。Device 代次变化和 Host 真正断开清队列、按钮资格与控制；短时间没收到物理鼠标报文不等于鼠标离线。

物理端点按 1ms 调度；有空间才发起下一次读取，避免队列溢出覆盖。UART 每轮最多 32 字节或检查到 100µs 截止，不在缓冲满时等待；14 字节反馈必须有整帧空间。无周期 CDC 输出或每包诊断计数。USB 枚举期仍使用原库同步流程，不能把运行期协作调度解释成所有路径绝无延时。

`target.json` 是目标能力的单一来源。构建生成 `target_capabilities.hpp`，会话上限与 UART/反馈都从它取值。协议只保留布局、CRC 和语义，不再混放某块板的波特率/步长。当前旧反馈 flags=12 只能明确代表 100 / 2ms / 200；不允许随意填新参数却冒用该能力标记。新能力必须在设备适配入口增加正式映射后才能使用，PC 速度档位仍保存比例 ID。

新硬件接入顺序：确认 Host/Device 是否可同时工作 → 实现 MCU 时钟/通道与 Host/Device 端口 → 添加板资源 → 目标装配与能力声明 → 同一核心/协议测试及平台适配回归 → 编译资源检查 → 获授权后实机验证。不为未购买硬件创建声称可用的 CH32/Teensy 空实现；FPGA 可复用协议和测试向量，不直接套用 C++ 固件。

上游源码不直接编辑：四个 AVR 覆盖文件的原始与结果哈希写入锁文件，构建在独立暂存中应用。UHS 使用原版 1.7.0；v6.1–6.6 的断开复核、IRQ 探测和诊断补丁均不进入 v7。历史旧树在 `releases/pre-v7-source-20261022/`，其中工具只作取证，不再作为工作入口。

全仓 `pnpm check:architecture` 与 `pnpm test:architecture` 同时检查此依赖方向；正式 VS 项目数量保持两个。固件更换硬件不要求新建 PC 项目或让 Reader 变更协议。
