# Web 界面

本目录实现 Mousebox 的 React 桌面界面。Electron 外壳位于 `../desktop`，共享命令和展示模型位于 `../../packages/shared`。界面不会直接创建网络连接、打开串口或处理原始鼠标报告。

- `layout`：DIMO 风格主侧栏、标题栏和状态栏。
- `pages`：六个路由入口；设置包含五个分类。
- `features`：算法来源、学习画布、设置分类和日志列表。
- `components/ui`：基于 Base UI、CVA 的 shadcn `base-nova` 组件。
- `stores`：按领域接收事件，展示文字与原始业务值分离；日志有界。
- `styles/theme.css`：浅色与深色语义颜色。DirectX 收到 CSS 解析后的画布颜色。
- `i18n`：保留完整历史词典与动态 `%N` 模板，新词条集中管理。原始日志不翻译。

表单在编辑时保留草稿，失焦或回车后验证并提交；空文本不会转换为零。Reader 是否请求连接与是否真正已连接分别展示。导入或拟合不会自动启用模型，删除必须通过原生确认令牌。

ESP 的 Web 界面只提交画布几何、显隐与遮挡，不通过界面桥传输 Reader 人物帧或 GPU 像素。速度学习显示原生汇总的采集位置；完整原始样本与训练留在 C++。

运行 `pnpm --filter @mousebox/ui build`、`typecheck`、`lint`、`test` 验证。测试使用隔离的合成 DTO 和 jsdom，无前台窗口、设备或游戏输入。生产字体随包离线提供。
