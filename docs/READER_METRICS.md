# Reader 通信与 ESP 绘制统计

PC1.2.7，2026-10-21。本功能只使用现有 Reader 正式数据、时钟探测和本地 GPU 完成通知，不修改 Reader、控制算法、串口或固件。

## 底栏

保留左下角三点及原尺寸/间距，其右侧是本应用全部 Reader TCP 通道的总上行、总下行；右下角按实际订阅列表展示每条连接的名称、读取到绘制延迟和接收帧率。目前是 Living、Local，各自独立。Web 每 100ms 左右接收有界摘要，不接收逐包、Reader 帧或像素。

流量使用 Socket 实际成功的 send/recv 字节数，包括四字节记录长度、Hello/ACK、心跳、时钟探测和业务数据；包含解析失败前已经收到的字节，不计 TCP/IP 头、重传和其他进程。单位遵照用户要求为 kb/s：`过去一秒字节数 × 8 / 1000`，即 1 kb/s = 125 字节/秒。上下行以本机为参照，不把串口或 Electron 内部管道计入。无流量显示 0.0；原生桥断连显示 —。

接收帧率使用各会话最近实际收到的完整业务记录的本机 QPC 接收时刻，统计成功接受的新 capture 批次。心跳、时钟回复、基线重传和同一状态的重复通知不计数；在绘制前被最新场景合并掉的批次仍算已接收。这里是实际接收帧数，不是配置的订阅 Hz，也不是 TCP 包数。

读取到绘制延迟按连接独立计算：

```text
开始时刻 = clock.localAnchorMs
         + (batch.sourceStartNs - clock.sourceAnchorNs) / 1,000,000
单批延迟 = GPU完成通知的本机QPC毫秒 - 开始时刻
显示延迟 = 近1秒首次完成绘制的新批次延迟的算术平均
```

起点是完整批次读取开始，包含 Reader 读取、编码/传输、消费、等待、投影和 GPU 绘制时间，不是读取结束、中点、接收时间、RTT/2，也不是旧 state.gap 的 Living/Local 批次间偏差。起点来自跨主机四时间戳校准估计；悬停显示当前窗口批次数及最大时钟估算不确定度。通知时间包含原生线程唤醒延时，不能宣称为物理显示器扫描/光子时间。

没有有效时钟、时钟超过现有3秒探测有效期、起点映射到完成时刻之后、近1秒没有新批次完成绘制或 GPU 完成计时不可用时显示 —，不编造零延迟。按本机连接代次、Reader 实例、session、epoch 和 clock mapping 隔离；已断开的旧 GPU 回执不能进入新连接窗口。同一 capture 因重绘/缩放/主题变化只记录首次完成；首次缺时钟也不在随后重绘时补造一条测量。

## 原生完成通知与帧率

使用 D3D11 GPU fence 和完成事件。D2D EndDraw 后在同一设备命令流插入 fence；只为成功 Present 的图像保留回执，既有绘制线程由完成事件唤醒收取。没有等待 GPU 的阻塞调用，也没有新增固定绘制定时器或 FPS 上限。回执至多8项，极端阻塞时丢弃统计资格而不是阻塞绘制或无界占内存；驱动不支持 fence 时正常绘制继续，相关指标显示不可用。

ESP 画布右上角显示实际 GPU 完成帧率和裸观看人数数字。FPS 是该次绘图时前一秒实际完成 GPU 绘制的数量，不是 Web 动画帧或 Reader 订阅频率；图像不再绘制时自然保留最后图像中的文字，后台 DTO 的滚动帧数仍归零。观看人数使用已核验的当前 Local 字段，真实0保留，未知显示 —；不额外推断观看人数。Web 的对应文本仅保留屏幕阅读器可读版本，避免画布外重复展示。

网络、接收帧数、完成延迟和绘制计数采用固定1000个1ms桶保存近1秒，窗口边界分辨率1ms，内存不随帧率或运行时长增长。摘要聚合只在展示路径执行，不加入高频控制循环。网络/绘制通知使用独立的短遥测锁，不进行文件IO或跨 Web 逐帧推送。

接口依据：[CreateFence](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_4/nf-d3d11_4-id3d11device5-createfence)、[Signal](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_3/nf-d3d11_3-id3d11devicecontext4-signal)、[SetEventOnCompletion](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_3/nf-d3d11_3-id3d11fence-seteventoncompletion)。

## 左下方位环

由原生 DirectX 在 ESP 画布左下角绘制。圆心箭头代表相机，朝上代表相机水平朝向。每个人物只编码水平相对方位，所有标记均在同一条圆环上：

```text
dx,dy = 人物世界XY - 相机世界XY    // 双方先各自加世界原点
nx,ny = (dx,dy) / hypot(dx,dy)
x = centerX + radius × (-nx × sin(yaw) + ny × cos(yaw))
y = centerY - radius × ( nx × cos(yaw) + ny × sin(yaw))
```

远近、高差、FOV、俯仰和滚转不改变半径；同方向重叠是预期效果，不人为错开角度。包括镜头外和身后的人物，沿用 ESP 人物筛选、颜色、预选/锁定高亮及显示距离上限，始终排除自己。XY重合无法定义方向则不画，不编造方位；当前相机、人物或世界关联无效时清空标记，小窗口按空间缩放或隐藏方位环。全部数据和几何留在原生绘制链路。
