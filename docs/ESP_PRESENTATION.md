# ESP 绘制区域与叠加层

PC2.8.0。用户补充明确当前只在CS2 ESP开放。工具栏“绘制方式”可设置区域、截图策略和绘制引擎，默认仍为内嵌画布。三项由Electron的UI偏好唯一写入，不写Reader或盒子固件；只有声明外部窗口目标的游戏应用这些选项。PUBG不显示新控件，即使CS2之前选过叠加或CPU绘制，PUBG仍使用原内嵌画布和原显卡设置。

| 模块 | 责任 |
|---|---|
| UI `features/esp/EspOutputSettings` | 控件与占位状态，不传图像、不操作游戏 |
| Electron `SurfaceCoordinator` / `NativeSurfaces` | 全局偏好、遮挡、最小化、已校验父窗口与私有原生命令 |
| native `window/surface_router` | 内嵌与叠加两种输出的生命周期和同一不可变场景分发 |
| native `window/game_window` | 前台窗口的进程名、可用时Reader进程ID、物理客户区与可见性 |
| native `window/overlay_surface` | 自有顶层透明窗口、鼠标穿透、不激活、捕获策略和跟随放置 |
| render `composition_target` / `DirectXRenderer` | 透明交换链、显卡或明确的WARP、完成统计 |
| render `d2d_scene_painter` | 透明输出省略网格/背景/空场景提示，其余图元沿用同一绘制 |

## 放置与生命周期

叠加层是本进程独立Win32顶层窗口，不是游戏子窗口，不注入、不挂钩、不读游戏内存。游戏注册表通过非空windowExecutable显式选择支持能力，当前只有CS2；UI的externalEsp描述同样只由CS2声明。精确匹配可执行文件名及正式快照进程ID。只接受当前前台游戏，取客户区并转换到物理屏幕坐标，支持屏幕负坐标；边框、标题栏不计入。使用自己的DPI与透明DirectComposition交换链，投影纵横比临时匹配该客户区，不修改持久化投影设置。

WS_EX_NOACTIVATE、WS_EX_TRANSPARENT与透明分层窗口使叠加层不抢焦点、鼠标穿透；不注册游戏输入。16ms放置轮询只检查前台与边界，场景仍由原生最新槽独立触发，不新增固定绘制帧率。相同尺寸仅移动窗口无需重绘。WM_PAINT使用标准BeginPaint/EndPaint配对处理。本机真实桥接对照发现，原先仅ValidateRect的叠加层在隐藏创建后使低优先级状态推送停滞；完整paint处理后基线及切换组均恢复，双计时器保留。这是本实现的测试结论，不外推为ValidateRect在所有窗口中都不正确。官方处理示例见[WM_PAINT](https://learn.microsoft.com/en-us/windows/win32/gdi/wm-paint)。

离开ESP页、切游戏、遮挡对话框、会话撤销、主进程断开或退出时隐藏。ESP页保持活动时最小化软件可以继续显示叠加层，但只有游戏在前台才可见；原始输入采集仍停止。隐藏smoke/诊断单独禁止叠加层显示。销毁时先停止GPU线程和释放交换链，再销毁自有窗口。

只支持本机游戏窗口；远端游戏画面不能在本机直接覆盖。主要针对窗口化/无边框，未承诺独占全屏。前台跨屏DPI、真实游戏焦点与捕获路径留待用户手动验证；自动测试使用隐藏自有窗口，不操作游戏。

## 截图与引擎分开

“跟随显卡设置”沿用既有适配器及失败回退；“软件绘制（CPU）”明确选择WARP，不把它称为绕过桌面合成。两种方式最终均由Windows桌面合成。

“允许截图捕获”使用WDA_NONE；“请求排除截图捕获”只对叠加层使用WDA_EXCLUDEFROMCAPTURE，并核对Windows版本及实际GetWindowDisplayAffinity。请求失败明确报错并隐藏叠加层，不静默声称已排除。该系统能力要求Windows10 2004及以上且DWM运行；不能保证所有截图或录屏方式，也不防外部相机。官方依据：[SetWindowDisplayAffinity](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity)、[CreateSwapChainForComposition](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforcomposition)。

## 后台验证

原生隐藏测试核对自有窗口身份/样式、不改变前台、实际系统捕获策略回读、WARP与默认显卡交换链的透明像素及预乘颜色、销毁，以及隐藏/最小化/区域切换意图。Web核对默认值、独立选项、偏好落盘、错误状态和隐藏显示规则。Electron隐藏smoke通过真实桥往返三项偏好并恢复，叠加层始终不可见、不采集游戏或桌面；结果见PROJECT_MEMORY最新记录。
