# Reader 本地人物被观战人数契约

PC0.6.1，2026-10-15。此版仅更新消费协议兼容和界面版本号，保留Qt Quick改造、主题/语言、原参数和输入逻辑；未增加观战人数展示或控制业务。

来源G:/pubg-hyperv-memory-reader/protocol，正式生成目录R:/pubg/analysis/reader/manual-update-2610.1.1.57-20261015/protocol-spectators-generated，libprotoc35.1，97消息/502字段。descriptor SHA256：746609C5BD303C01FCE7F2F775A42C87C9405CFB881FB991F28E7550B593EF7D。同步完整10proto、descriptor/manifest与30份上游纯消费源码；本次相对上一冻结快照实质变化为field_presence.hpp、生成schema.hpp和state.cpp，源码保持上游原样，清单重新核对。D485历史契约备份于out/reader-spectators-20261015/previous-reader-protocol与previous-reader-consumer。

## 字段语义与严格校验

- CharacterStateValue新增optional uint32 spectated_count=7，PropertyGroup3不变，present_fields增加位64，组允许掩码从63变为127；字段8仍拒绝。
- 按Reader约定，仅本批已核验本地身体提供，表示本地人物被观战人数，与当前ViewTarget或自己正在观战的对象不同。本项目不派生新的业务含义。
- 原生数据是非负int32，因此接收范围为0–2147483647。虽然protobuf类型为uint32，更大的uint32仍由状态验证拒绝。
- 0是存在且已读取的值，按optional字段显式编码并保留位64；缺失不补零。当前缺失或读取失败通过CLEAR撤销旧值，固定基线重建不得借用旧人数。
- 角色、骨骼、原生运动和跨机时间契约不变，既有血量19删除与武器4/5证据规则保持。

## 验证

Debug/Release各35项C++新检查覆盖optional零编码、缺失、合法最大值、数值类型及范围、位64、固定基线SET/零SET/缺失CLEAR/失败CLEAR、未知字段8拒绝，并运行既有严格协议、投影、运动二进制夹具和时钟回归。

官方Python protobuf7.36.2动态加载同一descriptor，独立构造9组数据再交C++严格解码：0/5/int32最大值/缺失通过，两个uint32越界、值与掩码不一致两种情况、未知字段8拒绝；另核对4份C++生成SET/零/CLEAR记录。两配置五场景TCP回环及既有Qt Quick隔离回归验证继续通过。

证据位于out/reader-spectators-20261015。数据为合成记录和本机回环，未连接真实Reader/TTL、开始采集、启用修正或发送鼠标输入，不能推断观战人数实机采集效果或新Reader部署状态。

中间PC0.6.1已构建但未打开；用户新增ESP页后最终合入并打开PC0.7.0，最终Debug/Release严格契约及9组独立protobuf复验继续通过，接收端二进制包含127掩码更新。Reader上游最终通知其远端PID12876、游戏PID21816，EXE SHA256 AB6E50F917BDBB4A5EA173FDB710F4C35BDEFD5AE527F5BA8E2444D5FCAF8FE9，descriptor不变；这是上游运行通知，不扩张为本项目独立实机核验。
