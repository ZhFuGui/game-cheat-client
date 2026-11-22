# 新增游戏模块

当前 PUBG 保留既有完整能力，CS2 提供独立 Reader v1 只读数据与矩阵 ESP，尚未开放瞄准；GTA5 仍未接入。capabilities 仅列实际实现功能，不能以游戏可选代替某项能力验收。

1. 锁定已有的正式来源契约、身份、时间、坐标轴、单位和缺失资格，不请求本窗口的 Reader 新增字段，不假设不同游戏共享协议。
2. 在 `adapters/sources/<source>` 实现来源适配，SDK/字节解析留在此处，向游戏提供有证据的数据。
3. 在 `games/<id>` 实现明确类型的模型、坐标转换、相机/骨连接、目标资格及游戏输入映射；有效数据转换为 `domain/scene`。未知单位、不同空间或失效身份拒绝使用。
4. 实现 `GameModule`、`DataSource`、`GameConfiguration` 及相应诊断、校准、学习/控制用例适配。非 PUBG 必须提供独立 GameApplication；通过 OutputBinding 使用唯一全局设备所有者，不能复用 PUBG Frame、诊断或设置解析器。
5. 提供 `SceneProjection`、自己的 `ProjectionState`、纯 `Primitives` 和 `SourceSample`。投影缓存只归唯一绘制线程；GPU 层不增加游戏头文件。
6. 更新游戏目录、宿主 composition、配置注册器和 UI 模块注册。全部完成并验证后才标 available；此时原生自动初始化唯一配置，侧栏自动增加一个正式游戏选项，不引入同游戏多配置；切换必须核验装配模块 ID。设备能力和公共鼠标资产不复制到游戏目录。
7. 按 [UI 接入说明](ADDING_A_GAME_UI.md) 组合共用页面与专用面板。未实现能力显示原因，不提供可点击假入口。
8. 补单位/轴、空间和代次、身份复用、缺帧、旧消息拒绝、配置迁移、失败重试、模型绑定、纯投影和关闭清理测试。先离线/隔离验证，再由用户验收真实游戏与手感。

数据布局见 [WORKSPACES](WORKSPACES.md)，依赖方向由生产架构检查和正反例约束。新增游戏不复制整套主程序，不增加页面级 VS 工程。

CS2 的正式接入边界、限制和验证见 [CS2 Reader 接入](CS2_READER_INTEGRATION.md)。通用带长度记录的 TCP 字节传输在 platform/windows/framed_socket；时钟、版本和业务规则仍归各自来源适配器。
