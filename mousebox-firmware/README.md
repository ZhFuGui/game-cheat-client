# Mousebox 固件 v7

v7.1新增可协商的有限左键点击，配套PC2.8.7 Triggerbot；当前已于2026-10-29烧录并全量读回通过，EEPROM与Bootloader保持。旧PC不请求新能力时沿用旧回传格式，物理鼠标透传与运动参数保持。协议和输出边界见[自动扳机说明](../docs/TRIGGERBOT.md)，设备收据见`backups/before-v7_1-triggerbot-pnp-20261029/operation.json`。

固件与 PC 工程分开维护。v7 已将协议、控制、接收器格式、MCU、Host/Device 和板资源拆开，完整结构及移植规则见[架构说明](docs/ARCHITECTURE.md)。物理鼠标透传可独立运行；整个项目仍以 Reader 为上游。

当前仓库根为 `G:\mousebox-aimbot`。PC2.0.0迁移保留本目录正式固件与固定v7产物原字节，没有再次烧录；旧仓库只供归档，不能继续作为开发入口。

当前生产目标：`leonardo_max3421e_c54d`，适用于 16MHz ATmega32U4 / Leonardo R3 + MAX3421E USB Host Shield + 罗技 046D:C54D 接收器。不是所有 32U4 板和接收器的通用镜像；未实现其他硬件时构建直接拒绝，不猜测能力。

v7.0.0 清理了 v6.1–6.6 的周期 CDC 诊断、Host 健康读取、断开寄存器采样及实验性 USB 修补。保留原生 CDC 维护/烧录入口，不主动打印；保留真实断开清理、CRC、控制租约、预算、松键门控、物理报文 FIFO 和 HID++ 转发。Host 库回到锁定的原版 UHS 1.7.0。**这次不继续排查硬件，也不宣称静置异常已解决。**

| 项目 | 当前目标 |
| --- | --- |
| UART | 500000 / 8N1 |
| 控制间隔与步长 | 最小 2ms；每轴 ±100 计数；单会话预算 200 |
| 物理端点调度 | 1ms；不逐包经过 PC/Web |
| 按钮反馈 | 14字节、主动握手后开启；正常 20ms 心跳 |
| 命令协议 | v1 / 24字节 / CRC16，能力 flags=12 |
| PC 兼容 | 当前 PC2.0.0 已支持，原 PC1.2.8 能力保持；速度档位继续按握手能力比例计算 |

调度频率和模拟回归不等于实测物理回报率，静态 SRAM 剩余不等于栈余量。

从仓库根目录执行：

```powershell
# 全部离线测试；只编译/运行假对象，不打开设备
mousebox-firmware/tools/test.ps1

# 当前硬件编译；依赖哈希核验后在 build 内创建隔离平台
mousebox-firmware/tools/build.ps1 -Target leonardo_max3421e_c54d -BuildName release-v7

# 使用同一应用和核心的离线模拟目标
mousebox-firmware/tools/build.ps1 -Target simulated_c54d -Product simulation

# 只枚举端口
mousebox-firmware/tools/device.ps1 -Action List
```

真实烧录入口为 `tools/device.ps1 -Action BackupAndUpload`，必须显式提供 `-Target`、`-RuntimePort`、`-ExpectedLocationPath` 与 `-ArtifactManifest`。先核验目标/能力/HEX范围和 SHA256，再备份并验证旧完整 Flash、读取 EEPROM，随后只写应用区；自动校验后再读完整 Flash/EEPROM，独立确认应用、引导区和 EEPROM。失败不盲目重试，不写 fuse、引导程序或 EEPROM。

固定发布位于[当前 v7 发布目录](releases/native-v7-clean-20261022/RELEASE.md)，设备操作结果另写 `deployment.json`，不篡改发布准备清单。实际是否烧录成功以部署收据及[交接](HANDOFF.md)最新记录为准。

旧版本、备份、日志与当时验证范围原样保留。重构前工作树移到 `releases/pre-v7-source-20261022/`，旧工具不再作为工作入口；旧说明副本见其中 `README-before-v7.md`。正式工具集中在 `tools/`，按需观测在 `tools/diagnostics/`，恢复原语在 `tools/recovery/`。旧 C 盘 Arduino 目录仅为历史副本。
