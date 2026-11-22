# AVR USB 覆盖层

对应固定 ArduinoCore-avr 1.8.7。`dependencies.lock.json` 对原始文件与完整覆盖文件均固定 SHA256；构建仅覆盖暂存平台，保留上游许可证及版权注释。

- `USBCore.cpp`：固定接口布局、非阻塞整包提交、设备代次及协作 EP0 Host 桥。
- `PluggableUSB.cpp`：接收器三个接口的注册布局。
- `CDC.cpp`：独立维护端口与 1200 波特率启动入口。
- `USBDesc.h`：端点与接口定义。

这些是 USB 运行所需适配，不是硬件故障诊断。生产应用不打印 CDC 日志。UHS 库没有覆盖层，直接使用锁定的未修改上游文件。
