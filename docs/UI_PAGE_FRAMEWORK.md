# UI 页面框架

PC2.8.37：`PageTab.accessory`承载属于单个标签的独立操作/说明；`PageScaffold.accessories`按稳定section ID装配到每项。操作与tab按钮同处`.page-tab-item`内并列，视觉上在标签内部右侧，避免嵌套按钮；拖动变换及层级作用于整个容器。未选中项的说明仍可查看，点击说明不选页或排序。

当前子页系统仅呈现一层标签。左侧导航选择功能入口，一级页签切换主要内容；同一内容的呈现方式使用内容区按钮，参数使用表单，操作使用工具栏。用户于2026-10-29明确撤回嵌套页签方案，原递归布局、父子位置观察和下级专用样式已移除。

READER 保留“连接 / 数据看板”一级标签，看板内“摘要 / 源数据”使用按钮。鼠标移动调试的“速度学习 / 鼠标特征”和 CS2 Recoil 的两项功能也使用同一一级框架。设置分类与 ESP 参数分组保持各自用途。

Visuals 通过同一框架提供「常规」「ESP」「Radar」。常规留空并固定首位，侧栏默认打开 ESP，旧 `/esp` 路径和 `--esp` 启动入口也进入 ESP。Radar 通过游戏 UI 模块延迟挂载，当前仅 CS2 接入，见 [CS2 Radar](CS2_RADAR.md)。切离 ESP 时由组件卸载释放画布和 Insert 快捷键，返回时重新挂载，并读取保存的 ESP 设置。

## 责任划分

| 位置 | 职责 |
| --- | --- |
| `framework/pages/model.ts` | 纯 PageGroup / PageSection 定义、校验、默认项、主路径解析 |
| `framework/pages/usePageRoute.ts` | 一级查询参数、历史导航、离开分区时清理其专属查询参数 |
| `framework/pages/PageScaffold.tsx` | 一排页签与一个面板、懒加载、非活动内容卸载 |
| `framework/pages/PageSession.tsx` | 当前工作区内的少量界面模式记忆，不缓存业务视图或源数据 |
| `framework/pages/PageOrder.tsx` | 顺序端口与稳定ID恢复，和路由/游戏代次分离 |
| `framework/tabs/model.ts`、`order.ts` | 与React/DOM无关的固定区域、可移动资格、偏好恢复及受约束移动 |
| `components/shared/PageTabs.tsx` | 页签样式、ARIA、导航与排序操作 |
| `components/shared/page-tabs/geometry.ts` | 已测量槽位、不同宽度的拖拽边界与落点，复用纯排序策略 |
| `components/shared/page-tabs/useTabReorder.ts` | 指针拖拽生命周期、rAF变换预览与取消清理 |
| `features/navigation/PageTabPreferences.tsx` | shell侧全局偏好适配、乐观顺序、保存失败恢复及遮挡owner |
| `navigation/pageGroups.ts` | 一级分组元数据，不导入实际业务组件 |
| `navigation/readerLocation.ts` | Reader 旧链接到一级分区/视图模式的纯转换 |
| `pages/*.tsx` | 将视图键和语义图标键绑定实际组件 |
| `features/data-dashboard/DataDashboard.tsx` | 看板模式按钮、模式历史与恢复、按需挂载摘要或源数据 |

框架和页面组描述不能导入游戏、业务 features、运行状态或原生命令；架构检查保证边界。各游戏数据面板仍由原有描述符选择，框架不解释游戏数据。

## 新增一级子页

```tsx
const group = definePageGroup({
  id: "example", route: "/example", label: "示例", defaultSection: "connection",
  sections: [
    { id: "connection", label: "连接", view: "connection" },
    { id: "data", label: "数据", view: "data", queryKeys: ["view"] },
  ],
});
const views = { connection: Connection, data: Data };
return <PageScaffold group={group} views={views} />;
```

同组 ID 唯一，使用稳定的小写字母/数字/连字符。空组、错误默认项和嵌套声明会被拒绝。视图绑定在组件外定义，支持 React.lazy；空占位直接不提供 view。正常页面不生成顶部大标题或简介，不留其占位；卡片分组、错误/等待/空状态保持。

## 看板视图与生命周期

- 主分区使用 `section=dashboard`，看板模式使用 `view=summary` 或 `view=source`。旧 `section=dashboard/source` 和启动参数继续进入源数据；旧 `/dashboard` 继续进入看板。
- “摘要 / 源数据”为普通切换按钮组，以 aria-pressed 表示选择，不构造第二个 tablist/tabpanel。按钮位于当前内容区域。
- URL 驱动浏览历史。没有显式模式时，恢复当前工作区最近选择并规范化 URL；显式的有效模式优先。换游戏或工作区代次会清空模式记忆。
- section 的 queryKeys 声明其专属查询字段；切离看板时清理 view，保持其他无关查询参数。看板自身缓存最近模式，返回时恢复。
- 只有当前分区和当前模式挂载。切回摘要、连接或离开 READER 会卸载源数据，停止120ms取样并隔离在途旧回复。不会在后台保留隐藏树，也不会因切页重连 Reader 或切换控制模式。

## 样式与验证

一级页签保持本体31.45px、可见留白4.05px、整栏35.5px，文字和宽度不缩放；吸顶始终为0，没有层级偏移。浅色基础页 rgb(250,251,250)，未选中页签 rgb(221,227,233)，深色保持原配色。颜色来自 theme.css 语义变量。

模型测试覆盖默认项、旧链接和拒绝嵌套；渲染测试覆盖只有一排标签、内容按钮、模式记忆、历史、卸载及工作区重置。源数据测试继续核验按需拉取与离页停止。隐藏 Electron 核对真实一排页签/两个模式按钮及底色连接，预览只捕获其自身页面，不操作桌面或游戏。

## 拖拽排序

所有可移动的一级标签共用拖拽排序。按住标签水平移动超过6px后开始预览，邻项通过transform让位；松手提交顺序，拖动本身不会选中标签或重挂载当前内容，点击仍正常切页。Esc、pointercancel、失焦、捕获丢失、声明顺序/固定规则变化及卸载取消，清理全部监听/待执行动画、指针捕获、光标及独立遮挡owner；窄窗口拖近两端时允许横向滚动。尊重减少动画偏好。

PageOrderProvider由shell提供数据和提交函数，框架不访问运行状态或命令。全局`preferences.pageTabOrder`按稳定group ID保存ID数组，Electron唯一写入`%LOCALAPPDATA%/Mousebox/web-ui.json`；与主题/侧栏同属界面偏好，跨游戏、离页和重开保留。单次更新按组合并，不能覆盖其他组；快速连续提交保留最新草稿，迟到成功不能替换新顺序，失败恢复最后已保存值并提示。旧偏好缺此字段沿用声明顺序；删除/未知ID过滤，新可移动标签追加，不改变默认section和URL。

方向键/Home/End按当前视觉顺序导航；Alt+Shift+左/右移动焦点标签，焦点及当前面板保留。元数据声明顺序仍作为无保存值时的初始顺序。Reader看板内摘要/源数据仍为普通按钮，不纳入页签排序。

交互测试覆盖拖放、短点击、其他指针、取消路径、键盘、当前面板未重建、持久化与保存竞争/失败；桌面契约测试覆盖按组合并、非法ID/重复项拒绝及重开恢复。隐藏整包验证用自身DOM的合成PointerEvent完成真实布局排序，核对选择/面板保留、磁盘重读与离页恢复，不发送桌面鼠标事件。

## 固定区域的声明

`PageSection`及独立`PageTabs`共用`TabDefinition`。未声明`placement`为可移动标签；`placement: "start"`固定在开头，`placement: "end"`固定在结尾。同一固定区域内按声明顺序排列，保存的偏好只决定中间可移动区域的顺序。

```ts
sections: [
  {id: "general", label: "常规", placement: "start"},
  {id: "main", label: "主要功能", view: "main"},
  {id: "extra", label: "扩展", view: "extra"},
  {id: "about", label: "关于", placement: "end"},
]
```

偏好恢复、拖拽落点及键盘排序都调用同一策略；读取旧偏好即按当前声明恢复固定区域，不在挂载时被动写盘。固定标签正常参与点击、方向键和Home/End导航，但不启动指针捕获、拖拽遮挡或排序快捷键。其他标签不能在预览中占用固定槽位，也不能通过提交不同的顺序将其挤走；无效或无变化的排序不保存。固定规则在拖拽中改变立即取消，恢复原样并释放owner。不同宽度按实际槽位边界计算，宽标签也能到达可移动区域端点。

当前Aim的“常规”声明为第一个`start`标签，在CS2/PUBG中都固定首位。PC2.8.32初次交付为空页；PC2.8.33由游戏的`aimGeneral`注册表装配共享算法、移动、时序和输入/弹道模块，详见[Aim常规](AIM_GENERAL.md)。默认路由继续显式声明为Aimbot；视觉位置与默认进入项分开控制。常规页使用独立`aim-general`上下文，避免显示另一子页的状态。内容模块及具体算法不进入通用页签框架；新增固定入口无需在拖拽、偏好或框架内识别业务ID。

页签全部禁止选择文字。鼠标焦点无蓝色轮廓；键盘焦点保留中性文字下划线。固定标签同样遵循此规则，可移动标签另保留`touch-action: pan-y`。原蓝框在旧轮廓样式的隔离对照可生成同形画面，用户最初偶发操作路径尚未复现，不能把该对照当作已确认的原始触发链。
