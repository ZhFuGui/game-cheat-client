# Mousebox 代码架构

PC1.2.0，2026-10-18。本次为保行为的结构重构；协议、配置格式、算法参数、设备时序和页面外观不随模块拆分改变。

PC1.2.1 将鼠标学习/特征的 Raw Input 接收端移入拥有前台窗口的 Electron 进程，使用现有 native 工程生成的 `mousebox-input.node`。Node-API 只负责打开、检查和关闭C++端点；专用接收线程、缓冲读取和原始记录均为C++。分段/完成数据经独立的本机C++二进制通道交给后台 RuntimeService，不经过Web或逐报文JS。Reader、串口、控制和DirectX仍在原后台宿主，正式VS项目数不变。

PC1.2.2 的页面上下文栏位于 `ui/src/features/page-context`：`model` 负责身份与页面归属，`facts` 选择当前页面需要的展示事实，`messages/faults` 生成当前有效信息，`scheduler` 决定去重、优先级与轮播资格，hook 只管理有限页面游标和计时，组件只做翻译和展示。顶栏不发起新请求，不订阅原始报告，也不解析本地化文案猜测故障。底部三点继续承担全局连接概况。完整规则见 [页面上下文栏](PAGE_CONTEXT.md)。

## 依赖方向

PC1.2.7 的 Reader 统计由 `reader/transport` 计量实际成功收发字节，`application/diagnostics/reader_metrics` 独立保存各连接的接收批次与滚动窗口，`render/gpu_completion` 用非阻塞 fence 事件回传绘制完成元数据；`runtime_service_snapshot` 只在展示路径汇总，`ui/features/reader-metrics` 只展示摘要。高频控制快照不做统计扫描。ESP 方位环几何在 `render/esp_bearing`，观看人数和绘制帧率在原生画布内，Web 只保留可访问文本。无新增固定绘制定时器、逐帧Web流或Reader协议变更。定义与误差范围见[通信与绘制统计](READER_METRICS.md)。

PC1.2.3与native-v6-500hz将TTL改为500000/8N1、PC控制最小2ms、各轴±100。v7 将共享线定义统一到 `mousebox-firmware/protocol/include`，能力数值由目标配置与 PC 的正式握手映射分别约束；平台串口接收调用者指定的波特率。设备写线程不等待凑批，将相邻 START/MOTION 写成一个48字节块。固件先服务物理USB，再做有界UART，物理FIFO/描述符/1ms轮询保持。参数与验证边界见[控制链路](CONTROL_LINK.md)。

PC1.2.4取消了PC1.2.3对v6的单一绑定：`device/connection_probe.hpp`负责已知速率的有界无运动握手与线速约束，`control/button_feedback.hpp`把已有能力标志投影为`DeviceMotionLimits`，`control::speedPolicy`仅按比例和能力值计算策略。`SerialSender`按实际能力构造单步并在入队/写出复核，输出循环遇能力变化先停止、重建控制器并要求松键。平台SerialPort只提供空闲状态下切速的通用字节接口，不依赖盒子协议。Web通过speedPresets变更订阅接收原生计算的比例、间隔及速度/单步/加速度，保存值仍为稳定档位ID；断开时清设备身份并明确标注v5参考值。未来增加能力版本集中扩展适配入口，旧协议无法报告的未知数值不猜测。v5可继续使用本版，无需先烧录v6。

```text
React 页面 → feature 展示 / 会话 hook → runtime 状态与事件 → 受限 desktopApi
                                      ↓ 契约
                              packages/shared（Zod / DTO）
                                      ↑ 契约
Electron 入口 → 启动装配 → 命令路由 / 窗口 / 存储 / 原生客户端
                                                     ↓ 本机进程管道
native 入口 → host 装配 → RequestLedger → RuntimeService
                       ↘ Win32 输入 / 原生表面        ↓
                                      application 协调与纯展示投影
                                       ↓ Reader / device / render
                                           ↓ control / protocol
                                                ↓ domain / platform

固件：sketch 装配 → USB接口注册 / Host传输 / 报文管线 / 控制接入
共同的线协议定义位于 mousebox-firmware/libraries/MouseboxControl。
```

这里的箭头表示允许的依赖方向，并不表示 Reader、输入、绘制逐帧经过所有层。Reader、Raw Input、串口和 GPU 高频工作始终留在原生线程；Web 只处理命令和有界展示 DTO。

## 源码导航

```text
apps/
  desktop/src/main/
    index.ts                       单实例与启动失败处理
    application/
      launchOptions.ts             解析启动环境
      launchDesktop.ts             桌面应用装配与生命周期连接
      container.ts                 依赖装配，不承载业务分发
      commandRouter.ts             白名单命令、迁移资格、路径资格、分发顺序
      rendererEvents.ts            校验事件、最小化合并、有界日志缓存
      surfaceCoordinator.ts        文件对话框遮挡与表面恢复
      startup/                     顺序导入、报告、校准、诊断任务
    verification/                  仅隔离隐藏验收：导航/鼠标布局/设置/退出
    window/                        窗口、来源安全、会话租约、原生表面
    infrastructure/                管道客户端、迁移与偏好存储
  ui/src/
    pages/                         页面组合与导航
    features/
      learning/                    会话 hook、采集/模型/确认展示
      mouse-debug/                 原始采集事务、完成结果和图表
      esp/                         设置展示、Insert交互状态
      aimbot/                      参数/运行状态/来源说明展示
    runtime/                       纯状态、订阅连接、本地化投影
    stores/runtimeStore.ts         原公开入口的兼容导出
    styles/index.css               唯一有序样式入口
    styles/                        基础、布局、控件与领域样式
  native-host/
    main.cpp                       CLI/桌面宿主选择与异常出口
    host/                          装配、窗口消息、场景推送、宿主命令
    bridge/
      transport.*                  有界帧读写
      request_ledger.*             会话封套、请求身份、有限窗口内重复回复
      runtime_service.*            会话生命周期和阶段协调
      runtime_service_*.cpp        设置/连接/学习/模型/报告/日志/快照命令
    input/
      addon/                       前台进程内 Node-API 装配，无逐报文JS回调
      raw_capture.*                专用接收线程、资格和光标范围所有权
      raw_input_buffer.*           批次边界核验和逐记录读取
      raw_registration.*           保存/恢复本进程原始鼠标注册
      capture_server.*             前台C++端点、完成队列和取消
      capture_client.*             后台RuntimeService的窄采集端口
      capture_channel.*            本机管道、身份、帧上限和可取消IO
      capture_payload.*            完成数据的二进制编码/核验
    window/                        原生DirectX子窗口
packages/shared/src/
  models/                          按配置/运行/学习/报文/绘制等领域划分DTO
  contracts/                       各领域命令与事件契约
src/
  application/
    runtime/                       Backend协调与Reader/设备/配置/校准协作
      runtime_models.hpp           只读快照与配置类型，不依赖可变Backend入口
    tasks/background_queue.*       有界FIFO、任务失败、关闭排空
    diagnostics/                   纯状态/连接/算法/日志展示
    storage/                       JSON、模型、样本与枪械资料
  reader/
    control_projection.*           正式状态到内部Frame的投影入口
    detail/wire_facts.*            类型访问、当前性、身份与嵌套采样资格
    detail/local_projection.*      Local武器及原生观测的投影
  render/
    graphics_adapter.hpp           显卡信息模型
    dxgi_adapters.*                 显卡发现、标识、排序
    directx_renderer.*             设备、资源、绘制工作线程
    directx_error.hpp              共用HRESULT诊断
    esp_scene.*                    无GPU依赖的ESP几何
  platform/windows/                通用Windows能力；串口写入接收字节span
  device/                          盒子发送与反馈，不把线协议推入平台层
  control/                         目标/控制/轨迹/弹道计算
  protocol/                        线协议编解码
  domain/                          无Windows、Web和存储依赖的数据与规则
mousebox-firmware/
  protocol/include/               PC/固件共用线协议
  core/                           无SDK依赖的控制/管线/端口契约
  profiles/                       接收器格式与USB描述符
  platforms/                      MCU、Host/Device、板资源适配
  apps/relay/                     共享运行调度与薄Arduino入口
  targets/                        能力清单与最终装配；含离线模拟目标
  third_party/                    原始固定依赖与显式USB覆盖层
```

正式解决方案仍为 `mousebox-core` 静态库与 `mousebox-native` 宿主；本地解决方案增加 `mousebox-tests`。拆分的是职责与源码，不增加 VS 工程。原生与 core 通过现有目录通配编译，新 runtime service 实现同时纳入测试工程。

## 所有权与并发

- React 页面不拥有全局订阅；`runtime/connection` 管理连接代次及退订，`displayProjection` 保留未翻译原值。业务 hook 负责本页开始/停止/文件选择；展示组件不直接操作设备。
- 原始报文采集 hook 先同步页面布局、确认最终原生区域，再请求开始。切页期间尚未完成的区域确认不能补发开始。完整报告保留在 C++，图表只接收完成后的摘要。
- Electron 的命令路由保留迁移拦截、文件路径白名单和用户动作先后顺序。事件发布负责最小化时合并状态及4000项日志上限，避免各页面重复实现缓存。
- Host 消息线程唯一拥有 `RequestLedger`。重复请求仅在64项/8MB保留窗口内去重；超大回复不缓存。命令回复与事件共享会话代次，不能把缓存重放当作续租或重新赋予输出资格。
- `RuntimeService` 是业务会话状态唯一协调器。学习、模型和报告各自聚合状态；命令实现共享同一私有上下文以保留跨域互斥的原子性，不通过多套锁复制所有权。
- `inputMutex` 只用于原始采集交接，持有时不能取得运行时状态锁。后台队列不占用输入线程；停止和排空不能持有任务完成回调所需的状态锁。
- `BackgroundQueue` 是单次生命周期的有界FIFO；最多4项等待任务，标准任务异常交给调用方，已接受任务在关闭时排空。调用方错误处理不得抛异常。
- `RuntimeLog` 由调用方状态锁串行访问，最多4000项；清空记录不复位单调游标。只读状态投影接收快照和显式时刻，不打开设备或保存文件。
- GPU资源仍由唯一绘制工作线程拥有；显卡枚举独立于渲染设备创建，最新场景合并、无固定FPS上限和WARP回退保持。
- C++是业务设置/采样/模型的唯一写入者；Electron是主题、语言、窗口偏好的唯一写入者。旧配置来源与迁移优先级不变。

## 复用与扩展约束

新增界面功能先定义共享契约，再接主进程路由与原生命令；不得把Node能力透传给React。shared只依赖契约和数据工具，不反向导入应用、React或Electron。聚合入口保留是为了兼容调用者，不在其中堆积新实现。

原生领域模型不依赖Win32、GPU或持久化。Reader字段访问和当前性证明留在adapter，控制层只消费内部模型；不得为了少写字段访问代码，绕过各组独立采样窗、实体身份或来源资格。Windows串口接收字节span，盒子协议的帧尺寸和编码由device/protocol承担。

文件按“什么原因会一起变化”组织，不按任意行数切分。已经独立的控制数学、Raw Input线程、线协议和物理传输状态机不额外套空接口；冻结Reader SDK、第三方源码、历史固件发布与用户数据不参与格式化或结构重写。

## 检查入口

```text
pnpm check:architecture             生产源码依赖方向
pnpm test:architecture              边界检查器的正/反例
pnpm typecheck / pnpm lint          Web类型与静态检查
pnpm test                          边界检查与Web/真实管道回归
tools/build.ps1 -Configuration Debug|Release -IncludeLocalTools
out/x64/<配置>/mousebox-tests.exe --suite all
mousebox-firmware/tools/test.ps1
mousebox-firmware/tools/build.ps1 -Target leonardo_max3421e_c54d -BuildName <隔离构建名>
```

`check:architecture`覆盖生产静态import/include，不替代类型检查、链接器或运行时权限校验；不解析生成代码、条件编译及动态计算路径。修改分层规则必须同时更新本说明和正/反例。`.clang-format`提供自有C++格式约定，第三方与冻结源码保持原貌。

既有PC解耦回归涵盖后台任务容量/FIFO/失败继续/排空、日志游标、请求重放、迁移和文件命令顺序、租约/关闭等待、采集卸载、导入不自动启用、订阅隔离与样式级联。隐藏验收使用合成数据，不声称替代真实设备、游戏操作、跨屏DPI和物理回报率测试。

2026-10-22 的固件 v7 是新的多硬件边界重构，同时移除硬件诊断与试验逻辑，HEX 不再与旧版相同。正式支持当前 Leonardo + MAX3421E + C54D，其他硬件只预留真实适配边界；详细依赖、目标结构、能力规则与测试见[固件架构](../mousebox-firmware/docs/ARCHITECTURE.md)。生产 PC 算法不读取目标文件，速度仍由握手能力按比例计算。历史v5保行为重构的验证结论仅适用于当时发布。
