# 原生应用服务

`apps/native-host/bridge/runtime_service.hpp` 是桌面宿主的业务入口。原生控制、Reader、串口及 DirectX 高频路径继续由 C++ 运行；界面只接收摘要。

- `handle(command, payload)`：明确动作的命令，不使用连接/激活状态翻转。
- `snapshot(includeDiagnostics)`：返回分领域 DTO；仅订阅算法来源时生成完整 30 项说明。
- `poll()`：合并 Raw Input 邮箱、后台拟合及文件任务完成结果，处理增量日志与采样自动保存。
- `backend()` / `espOptions()` / `values()`：供原生绘制宿主获取共享场景与显示配置。
- `setSessionEligible(false)`：持续撤销输出资格，而非仅发送一次停止。默认状态即无资格；侧键松开后的原控制门控仍保留。

配置使用数据目录的 `settings.json`，格式为 `{schemaVersion, values, espValues, profiles, speedSchema}`。配置存在时构造器不会覆盖用户值。`settings.import` 仅供用户明确选择迁移来源后调用，要求当前无 Reader/盒子连接和采集拟合。界面主题、语言和窗口偏好由 Electron 单独保存。

文件导入导出与采样自动保存使用独立、有界的文件任务队列，避免等待文件系统阻塞输入消息循环。`samples.import/export`、`model.import/export`、`recording.load` 和 `logs.export` 返回接受结果后，通过 `learning.ioBusy` 与 `status/feedback` 报告完成或失败；失败保留已有样本、模型和原始文件。模型载入与拟合均不自动启用。模型删除使用一次性确认令牌，复核模型代次、当前路径、规范化实际路径、受支持格式与请求确认时的内容 SHA-256。重新拟合、载入或导出会使旧令牌失效。

Raw Input 使用专用原生消息线程及独立 HWND_MESSAGE 窗口，逐包读取与消费端时间戳均在该线程内完成，界面和文件任务不共用该消息循环。start/stop/bounds/clear 通过有界命令队列同步投递，超时撤销并额外停止采集；线程退出会解除所有等待的命令。回调通过独立小锁邮箱移交采样段和稀疏视觉位置。`appendCapturedTrials` 返回 false 表示累计 1000 段或 200 万点的上限，宿主必须停止采集并恢复光标。所有原始点留在 C++，只把视觉位置和摘要给界面。每次显式 start/pause 才改变独立场次；六秒分段不产生新场次。

Reader 链路状态取各通道 `Session::ready()`，断开回调立即清除，独立于缓存人物帧与控制资格。仍区分任务已启动、协议已连接、当前有效数据以及当前可输出。

Living 与 Local 分别记忆自己的 World/Level 上下文；Living 不依赖新版协议已移出的本机控制字段。Local 控制字段独立读取失败时，仅保留上次控制器身份用于识别真正的切换，绝不把它补入当前帧。普通人物帧、时钟探测和同一控制器的数据恢复不递增控制代次；真实控制器、Reader/epoch/World/Level 或用户配置变化仍取消当前按住。

`tests/reader_runtime_context.py` 使用官方 Protobuf 运行库和冻结夹具，通过真实 C++ 进程管道及两路本机 TCP 验证 27 次采集：普通更新代次稳定、真实 Local 控制器变化递增、独立控制读取失败及恢复不误判。测试不取得输出会话资格、不打开串口、不采集或注入真实输入；夹具缺少运动条件，不宣称实机运动就绪。它已纳入 `tools/test-reader-loopback.ps1`。

弹道资料继续优先读取可执行目录的 `data/weapons/profiles.json`；仅文件不存在时使用编译嵌入资料。外部文件存在但无效时报告失败，保留当前有效目录，不悄悄更换资料。嵌入资料来源与 SHA 身份保持可核查。

原生应用服务回归见 `mousebox-tests --suite application`，覆盖最新参数保留、合法零值、校准数组、Unicode 原子存储、持续会话门控、资料失败语义和 30 项算法说明。另有实际 RuntimeService 异步模型导入/精确删除、关闭时排空文件队列、保存失败仍撤销资格，以及只创建隐藏输入线程的反复建销检查。该测试不连接设备、不创建前台窗口、不操作游戏；隐藏输入线程检查不会调用 start、注册 Raw Input 或限制光标。
