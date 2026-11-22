# CS2 Reader 冻结契约

当前随 PC2.8.47 构建的契约为 `cs2.reader.v2`，descriptor SHA256 为 `de589b63c5efe26492a267bafed709034f8ad31cec02990a3662a87eacaff727`。`manifest.json` 固定公开协议、descriptor、原因码目录及 SDK 的文件摘要。版本和摘要均通过校验后才发送订阅。TCP 记录继续用 4 字节大端长度，正文最大 16 MiB。

`Status` 使用可选的 `uint32 reason_code = 5`，缺省或 0 代表无详细原因。`Reject` 用保留了请求 ID 的报文承载非零 `uint32 reason_code = 4`。未知正数按原编号展示，`READY` 仅表明来源就绪，需等新快照更新数据。快照消息字段、时钟报文、后坐求值保持等价。详见[接入说明](../../../docs/CS2_READER_INTEGRATION.md)。

## v1 历史记录

以下摘要和运行信息属于已归档 v1，见 `manifest-v1.json`。旧 `tools/stage-cs2-reader-contract.py` 用于历史交接，当前构建验证由 `tools/check-cs2-reader-contract.ps1` 执行。

2026-10-30 源码已配对导入完整武器精度契约，当前源码冻结 descriptor 为 `f1905836aeff62574568e6534f866ba14bfee516b0d79a595b57b1412d748446`，交付包 `R:\cs2\sdk\reader-v1-f1905836aeff-accuracy-complete`。新增 `Weapon.accuracy_evaluation=15`、`Hello.weapon_accuracy_evaluation=14`；原 `accuracy_penalty=10` 和 `recoil.hpp` 保持语义与原字节。完整 Inaccuracy 与独立 Spread 均为无量纲方向幅度。下方地图版本段为历史范围，不是当前源码摘要。

运行包须完成双方验收后配对切换，不能把源码已导入当作运行服务已更新，也不同时接受旧摘要。85项上游合成记录中有21项精度专用样本；普通读取/脏速度/物理等失败独立撤销对应结果，值与状态矛盾、模型未知、必需成功分支字段缺失或区间越界则撤销整个新增组，旧快照保留。消费端只展示本地当前完整人物/武器句柄匹配的组，失效立即清空；不把该组用作瞄准、后坐或开火条件。实际部署结果见 `PROJECT_MEMORY.md` 与接入说明。

来源：用户审阅并授权配对接入的 `G:\cs2-reader\protocol\reader\v1`。当前配套交付为`R:\cs2\sdk\reader-v1-12883573c800-map`。本地三份proto、descriptor和公开SDK保持交付字节，部分上游注释仍保留候选阶段说明；实际运行服务与验证以[接入说明](../../../docs/CS2_READER_INTEGRATION.md)为准。

当前冻结descriptor SHA256：`12883573c8002e668bc7323c50b6cd61260387d01204267c4f3402818eff8038`（PC2.4.0，2026-10-25）。新增World.map_identity=13与Hello.map_identity=12；原冻结后坐求值器recoil.hpp原字节不变。本项目的独立受限wire codec不链接PUBG SDK或libprotobuf。构建执行`tools/check-cs2-reader-contract.ps1`，验证descriptor、proto及SDK摘要；37条官方记录夹具和真实管道另核对存在性/64位/请求编码与地图清空生命周期。旧PC2.3.0/0dd与新Reader须成对回退，不能放宽hash握手混用。

网络为 4 字节大端长度加 Envelope，正文 1..16MiB；UI 与原生的本机 JSON 管道仍是原有独立协议，不能混用。完整快照替换、明确失效、独立连接发布上限、时钟域与往返映射均按 v1 解释。不新增未核验字段、不猜测骨点角色、计数/度或后坐倍率。
