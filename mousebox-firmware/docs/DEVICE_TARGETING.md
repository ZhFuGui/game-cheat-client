# 多底板烧录目标核对

2026-10-21新增工具选项，固件协议及运行代码不变。

两块底板可使用相同的046D:C54D、相同产品名及固定序列号，不能仅凭VID/PID决定目标。`tools/device.ps1`仍先核验指定运行COM/USB接口；新增可选`ExpectedLocationPath`，在运行期和捕获bootloader后核对精确物理USB路由，除去复合接口的USBMI后缀。相似前缀、其他集线器分支、缺失路由均拒绝。

可同时指定`ExpectedBootloaderInstanceId`，必须配合物理路由。捕获时只接受这个完整实例ID，排除另一块板已存在的bootloader；实际存储操作前再次核对物理路由。未指定这两个选项时保留原行为，遇多个bootloader仍拒绝。串口号可以随驱动枚举变化，不作为底板永久身份。

当前本机这次接线的证据（不是其他机器的默认值）：

| 底板 | 运行期 | bootloader | 精确PCI/USB路由 |
| --- | --- | --- | --- |
| 新板 | COM12，MI03\6&36C7796F&2&0003 | COM10，2341:0036\5&242A2F40&0&1 | PCIROOT(0)#PCI(1400)#USBROOT(0)#USB(1) |
| 旧板 | COM17，MI03\7&3991DBB4&0&0003 | COM4，2341:0036\6&5E931A0&0&4 | PCIROOT(0)#PCI(1400)#USBROOT(0)#USB(4)#USB(4) |

这些值须由当前实机重新核验，不跨机器复用。21日22:35旧板单次尝试时，新板COM10同时存在；工具只捕获旧COM4，未复位/读写新板。旧板Flash/EEPROM备份和Flash复核成功，固定原始v6在写入起点45秒超时，未完成升级，没有自动重试。收据在`build/old-board-v6-attempt-20261021/`。

`tests/device-location-tests.ps1`模拟运行接口/bootloader路由、不同底板/集线器分支/缺失数据拒绝、关闭选项的旧行为，以及显式选择旧bootloader并排除另一块板；不访问真实设备。由`tools/test.ps1`一并运行。
