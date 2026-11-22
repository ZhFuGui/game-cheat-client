# DirectX 绘制

原生部分继续使用 Direct3D11 / Direct2D / DirectWrite，作为 `mousebox-core` 的组成部分。`apps/native-host/window/native_surface.*` 管理 Electron 的跨进程子窗口；React 不接收 GPU 像素，不运行骨骼绘制循环。

Reader 变化、失效、目标、尺寸、主题变化均通知最新场景；一个原生消息通知和一个绘制邮箱合并过时状态。工作线程事件等待，没有固定绘制定时器或 FPS 上限。`Present(0, DXGI_PRESENT_DO_NOT_WAIT)` 不主动等待垂直同步，最终可见频率由 GPU、数据和桌面合成决定，不能把输入 144Hz 说成显示 144Hz。

CSS 解析出的 canvas/grid/guide/text/entity/selected 语义颜色传给原生。位置按父窗口真实 DPI 转为物理像素；父 HWND 必须属于桌面进程，边界必须在客户区内。隐藏、最小化、模态遮挡不绘制，恢复画最新场景。隐藏子窗口创建不代表 D3D 设备已经初始化。

默认按 DXGI 高性能优先顺序尝试全部硬件；不支持该接口时按独立显存排序作为近似。显式选择保留设备指纹，缺失/失败时显示真实回退原因；硬件均不可用才使用 WARP。显卡选择不会改变控制配置或校准。

`mousebox-tests --suite render` 是纯几何回归；`--suite window` 显式创建全隐藏 Win32 父子窗口，测试 WARP/硬件、最新场景、隐藏、resize 和销毁；`--suite benchmark --seconds 1800 --output <path>` 使用 200 人×20 骨点、144Hz 合成输入，定期记录内存及 P95。没有 Reader、串口或用户输入。

隐藏窗口和合成测试不证明跨屏 DPI、前台焦点、游戏现场吞吐或真实 1000Hz。没有旧版同负载实测基线时，不声称 P95 回退小于10%。
