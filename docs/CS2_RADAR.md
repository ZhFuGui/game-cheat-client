# CS2 Radar

入口为 Visuals → Radar。内容区达到 700 CSS px 时，CT、地图、T 横向排列。低于 700 CSS px 时地图位于上方，两个名单在下方并排。列表展示同一批快照中的生命、护甲和当前武器，弹药按已有字段显示。已阵亡玩家保留在名单。当前层标记的方向线使用本批眼角，缺失时显示圆点。

默认俯视全图。滚轮或按钮缩放，可拖动画布或用方向键平移。跟随本地时以本地玩家为中心，有当前方向时将该方向转向画布上方。可以切换名字显示和重置视图。

底图及校准值来自用户指定的 `G:/cs2/cs2radar/public/maps` 与 `src/core/utils/map-configs.ts`。当前接入 Dust II、Mirage、Inferno、Nuke、Overpass、Vertigo、Ancient、Anubis。世界 X 向右，世界 Y 向上，使用参数标定的 1024 像素基准并按输出尺寸缩放。Dust II 的原图为 2048 像素，同样沿用这套校准。地图名和资源路径都要对应已知底图。

Nuke 和 Vertigo 默认按本地玩家本批位置的 Z 值选择底图。Nuke 分界为 −495，Vertigo 为 11700，达到分界值使用主层，低于分界值使用下层。本地位置缺失时沿用这次地图会话最近确认的层，尚无记录时显示主层。可手动选择主层或下层，之后用「自动」恢复。两个底图在打开地图时一起加载。 [Nuke 地图配置](https://raw.githubusercontent.com/SteamTracking/GameTracking-CS2/master/game/csgo/pak01_dir/resource/overviews/de_nuke.txt) · [Vertigo 地图配置](https://raw.githubusercontent.com/SteamTracking/GameTracking-CS2/master/game/csgo/pak01_dir/resource/overviews/de_vertigo.txt)

同层玩家使用实心圆，异层玩家使用带 ↑ 或 ↓ 的菱形，保持队伍颜色。异层图标发生遮挡时沿屏幕平面错开，细线连接到真实投影位置。切换底图后标记样式按相对楼层更新。

## 数据和生命周期

- `game.cs2.radar.read` 在当前 CS2 工作区内读取已接受的最新 Reader 快照，输出紧凑的地图与玩家名册投影。地图、队伍、位置、方向、生命、护甲和手持武器共用一批数据。武器通过完整句柄关联。
- `readRadarFrame` 保留缺失状态和明确的零值。明确死亡、休眠或缺少位置的玩家从地图标记移除。失效时原生返回空帧，即使请求带有上一帧的取样标识。
- 页面最多约 30 次每秒请求，上一轮完成后才发起下一次。相同取样标识只返回确认。切页、文档报告隐藏或切游戏时停止刷新并忽略旧请求，底图在地图代次变化时重新绑定。地图和人物通过独立 Canvas 绘制，现有 ESP 画布各自在激活的子页挂载。
- 当前只注册 CS2 的 Radar 页面，PUBG 保留预留页签。应用使用已有 Reader v2 连接和源快照。
