# 本机静止后响应异常的只读取证

2026-10-21。实机仍v6.4；此工具不烧录、不重启设备、不修改电源策略，不打开CDC或HID句柄，不需要管理员、前台窗口、鼠标注入或键盘监听。

实际新记录已完成：trial2在21:12:17—21:13:17共256条Raw Input、583份电源/光标，五节点全D0，用户反馈“难说”，不作首份响应通过/失败。trial1未对齐同步操作提示，不作受控响应验证。窗口结束后另行CDC记录复现Host释放，与此工具的采集阶段分开，详见[后续分析与候选](IDLE_CONFIRMATION.md)。

此前14秒日志期间用户在插拔接线，不能作为固定接线验证；用户描述的日常5秒静止现象也不是对该窗口的确认。原始计数保留，改善判断已撤回。后续先确认固定接线再明确开始新的试验，不将准备阶段或后台试跑当作用户复现。

`tools/observe-wakeup.ps1`使用PowerShell 7，复用`RawInputMonitor.cs`的隐藏消息窗口和设备过滤，只读取本机046D:C54D/MI00 Raw Input。新增可选观察回调默认关闭，不改变原`measure.ps1`的用法。数据包含：

- `*-raw.csv`：收到的每份Raw Input记录的QPC、来源、dx/dy；最多120000行，超出数量明确记录，不补点或倍率放大。
- `*-power.csv`：约100ms采样一次光标位置、盒子复合USB父设备/鼠标接口/原生CDC接口/连接Hub/鼠标HID的Windows最近电源状态。D0为工作，D1–D3为其他电源状态；读取失败保留错误码，不当成D0。只查询Configuration Manager属性，不打开设备数据通道。
- `*-metadata.log`：共同QPC原点/频率、本地日期、实例ID映射和最终计数；`*-summary.log`保留原监视器的每秒摘要。

这些是Windows交付与状态观察，不能获得人手开始运动的准确时刻，后台Raw Input也可能缓冲/合并。PowerData是Windows维护的最近状态，100ms轮询可能漏掉短转换，D0不能排除接收器、固件或短暂挂起；光标位置也不等于设备每份报文。此工具没有USB总线抓包或固件首包时间戳，不以它单独宣称丢包/硬件1000Hz/唯一根因。

持续打开CDC可能影响复合USB空闲条件，因此本工具全过程不打开COM12；也不为了验证猜测就禁用系统挂起。参考[设备PowerData](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-powerdata)、[CM_POWER_DATA](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-cm_power_data_s)和[HID电源管理](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/power-management)。USB ETW的官方步骤需要管理员，此次没有申请UAC、改变权限或启动该追踪。

先执行`-ValidateOnly`编译并检查只读访问，确认用户准备好后再运行有界采集。例：

```powershell
pwsh -NoProfile -File mousebox-firmware/tools/observe-wakeup.ps1 -ValidateOnly
pwsh -NoProfile -File mousebox-firmware/tools/observe-wakeup.ps1 -Seconds 60 -Label trial1 -OutputDirectory mousebox-firmware/build/idle-wakeup-followup-20261021
```

运行中保持原接线不动。记录启动后再通知用户开始静置约10–15秒并轻移一次，标明观察区间；结果必须结合用户的实际反馈。只读准备检查和2秒后台试跑已通过，试跑无鼠标记录，不能算已验证真实首份报文或故障重现。
