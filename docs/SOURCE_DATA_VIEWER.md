# 数据看板源数据

PC2.8.3，2026-10-28。参考G:/cs2/cs2radar/src/ui/desktop/views/JsonDesktopView.vue的最新槽、120ms取样和虚拟JSON树交互，在本项目React语义样式内实现；不引入Vue、WebSocket网关或第二条Reader连接。

PC2.8.4起入口为READER → 数据看板 → 源数据，原数据看板独立导航已移除；旧/dashboard与--dashboard兼容跳转至Reader的看板子页。连接和看板按查询参数选择，浏览器历史可恢复，顶部上下文栏同步切换。

数据看板保留摘要，新增“源数据”标签。可选实际订阅流、搜索字段路径/值、展开层级、暂停/继续与复制当前JSON。树只渲染可视行，展开状态跨新取样保留。只有打开源数据页且文档可见时拉取；最多一个未完成请求，完成后再等待120ms。暂停、切到连接子页、离页、桥断开停止拉取；换游戏/epoch/订阅清空旧树，忽略旧请求完成。原生故障显示在面板内，不反复弹提示。已暂停的数据明确标为历史取样。

## 数据来源

- CS2：使用当前连接已接受完整快照的真实Envelope正文，在消费者已有解码/资格校验之后移动进一个共享不可变最新槽。诊断不依赖SourceSample、Reader时钟校准或控制上下文；目录资格未满足的已接收快照仍可查看，但不会授予控制资格。失效状态、断连/过期不提供旧包。
- PUBG：分别展示Living与Local，使用SDK已按固定ACK基线合并增量的不可变Consumer State，而不是只显示最后一个delta，也不是PC控制Frame或页面摘要。借用固定SDK Consumer编码器构造诊断容器后去掉临时generation/fence，标明materialized格式；不声称它是Reader实际发送的一条新Baseline，不虚构发送时间。未增加PUBG字段、健康数据或采集。
- 字段名称/类型来自当前两份冻结descriptor。tools/generate-source-inspection-schema.py使用Python标准库生成诊断表，--check核验一致性；不修改协议、冻结SDK或生产解码器。CS2只显示报文中存在字段；PUBG使用SDK合并值及其默认值语义，不还原历史单条delta的字段存在性。保留真零/false；64位整数以十进制字符串保留精度，bytes以0x十六进制显示，非有限数以显式字符串显示，未知wire字段单列$unknown。

## 边界和开销

application/ports/SourcePacket只携带元数据及不可变read回调，适配器独占协议解释。SourceInspector有独立后台线程、一个最新工作槽和一个完成结果；仅按请求整理并序列化，保留原取样revision，撤销/换来源后拒绝旧结果。相同revision不重复发送payload。实时接收、控制和投影不执行JSON转换，业务运行仍完全使用原强类型快照。

诊断解析最多16MiB输入、200000字段节点及32层协议嵌套，展示JSON最多3MiB；超限明确报错，不静默截断。UI树最多400000行模型、64层JSON嵌套，实际DOM只有可视窗口和少量预取。最终桥编码/传输仍有诊断开销，仅在查看时发生；关闭后不会继续逐包把数据送入JS。原请求账本已有8MiB总缓存上限，桥帧8MiB上限保留。复制使用受限clipboard.write命令，仅响应用户按钮操作。

reader.inspect在三层既有reader.*工作区校验下执行，不能将前一游戏的数据显示到下一游戏。不存在的游戏/流不生成伪数据；控制资格、算法、串口、固件及Reader协议保持。原生/前端测试和隐藏Electron验证见PROJECT_MEMORY最新条目，隐藏验证仅用合成数据，不连接设备或改变剪贴板。
