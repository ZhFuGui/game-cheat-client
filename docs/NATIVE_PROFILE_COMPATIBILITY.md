# Native profile 2610.1.1.71 消费兼容

2026-10-21，**PC1.2.8已正式发布并最小化启动**。此前仅改应用谓词而冻结SDK仍拒绝新方向的问题，现已用正式配套消费快照解决；没有修改Reader字段或协议。

## 正式来源与冻结

来源为Reader正式包`reader-consumer-2610.1.1.71-24dba290-20261021.zip`，位置`R:/pubg/analysis/reader/local-receive-interruption-20261021`。

| 核验项 | SHA256 |
| --- | --- |
| ZIP | `d5a1fd98973acca9ed2d782161c62d081f9a1d19e9d8740b427f3e6acc4a591e` |
| descriptor（保持） | `24dba29046b068250a53544cf0648170ac0ff4bd82a631462e9053b00e3fd94c` |
| SOURCE_MANIFEST | `d05ea632b984a72e222d16df1d72bf87c688e103bd55e3acf340912c65121a78` |
| UPSTREAM_MANIFEST | `428be46e5efae6ba86096922edb5b5adae5bdfe2b3bfeb3ad8fc324f453ae558` |

完整包57文件，其中41纯消费源码/15CPP/10proto。对照旧快照，只有`synchronization/local_validation.cpp`两处精确profile字符串`.65`→`.71`，其余40源文件及全部proto/descriptor相同。源码按上游原字节导入，不手改SDK或只改哈希掩盖变化。自有README和IMPORT_RECEIPT与上游清单区分。

`stage-reader-contract.py`接受唯一的旧consumer-manifest或新UPSTREAM_MANIFEST格式，验证完整载荷、嵌套SOURCE清单、CPP列表及数量；双清单/哈希不符均拒绝。`check-reader-contract.ps1`同时固定两份清单SHA及现有源码/proto/descriptor校验。

## 分层兼容边界

正式新SDK的真实wire方向只接受以下精确来源：

| 分支 | 当前wire来源 |
| --- | --- |
| Barrel | `2610.1.1.71/barrel-zeroing-v1` |
| ViewNoTrace | `2610.1.1.71/view-no-trace-v1` |

应用层`domain/native_aim_profiles.hpp`仍允许各自分支的`.65`与`.71`，供既有内存状态和回归兼容。**这不代表旧`.65`方向报文也能通过新冻结SDK。** `.65/.72`、修订、跨分支、追加后缀的wire方向均按正式SDK拒绝。诊断保留原始profile，不把`.71`重写成`.65`。

Barrel仍要求阶段、归零及完整来源资格；ViewNoTrace保持观察分支，显式false归零、字段掩码、缺失/CLEAR/失败语义不变，不能获得Barrel控制资格。本次SDK接入未改变弹道、目标锁定、侧键、速度、150ms按钮新鲜度或人物短暂缺帧策略。PC1.2.8另有回传快照/取时钟竞态和连接诊断修改，固件v6.1另见固件文档，二者不属于Reader协议变化。

## 验证证据

- 原`.65`夹具目录保持原样；新`reader-native-aim-71-24db-20261021`明确标记合成，等长替换已知profile，其余字节不变，逐项记录新旧SHA，不冒充真实新游戏抓包。
- 84项真实`EncodeReliable`/`DecodeReliable`→`Session::accept`验证：`.71`正例、双分支、缺失/失败及版本/后缀/跨分支拒绝。先用旧SDK复现新夹具被拒绝，再用正式新快照通过。
- 官方Protobuf15个往返及10个负例通过；真实本机TCP→C++桥接10个Local状态覆盖方向、合法失败、缺失、CLEAR及恢复，无串口或输入注入。
- Debug/Release三工程及两配置`--suite all`通过；原生资格850检查/65场景、原生遥测200、连接状态39及既有控制/Reader/设备/几何/应用回归保持。
- 交接工具6项、Web/真实桥208项、架构4项/334源码、typecheck/lint、桌面构建及隐藏解压包smoke通过。smoke为version1.2.8/ok=true/visible=false/focused=false/capture inactive。
- 远端Reader只读12秒，Local/Living持续CURRENT，末尾1003/525更新，正常STOP清空。没有控制输出；该摘要未逐字段输出profile，不能单凭此短窗宣称当前真实游戏全部方向字段都有值或长期稳定性已验证。

证据`out/reader-sdk71-20261021`，冻结导入收据`third_party/reader-consumer/IMPORT_RECEIPT.json`。早期仅应用谓词的限定阶段仍保留在`out/native-profiles-71-20261021`和项目记忆历史块。

## 正式发布与数据

20:07正常换版274文件逐SHA一致，`out/desktop-release/win-unpacked`正式运行PC1.2.8，桌面快捷方式入口保持。旧PC1.2.7与两处最新完整数据在`out/upgrade-backups/reader-sdk71-20261021`。

用户最新Reader60Hz、gravity5.5、距离600、DPI1600及全部设置/校准逐值保留；两处各67模型/采样/调试文件逐SHA相同，主题/语言保持。启动自动连接Reader，两通道各60有效帧/s；最小化无绘制，延迟如实显示waiting-draw。无前台点击/截图、自动位移、Reader/Radar部署或字段变更，未提交/推送。

此修正解除已证实的SDK前置拒绝，不把所有历史Local断流都归因于它：原事故时刻缺少完整客户端异常证据。真实游戏控制与固件静止恢复另需用户接线验收。
