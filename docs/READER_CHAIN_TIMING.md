# READER 订阅链路计时

PC2.8.1，在READER页新增“订阅链路耗时”。本次适用于PUBG和CS2，按实际订阅TCP连接分别列行，显示近2000ms内有效新帧的算术平均；PUBG Living/Local分别统计，CS2保持完整快照的一条订阅，不虚构独立Local/人物时序。

| 阶段 | 起点 | 终点 |
|---|---|---|
| 读取耗时 | 完整批次第一次读取之前 | 最后一次读取及必要核验完成 |
| 发送准备 | 上述读完时刻 | 完成封装、第一次发送调用前盖戳 |
| 发送至接收 | 上述发送前盖戳 | mousebox-aimbot完整收到当前TCP记录、开始解码之前 |

发送准备包括CPU整理、提交、排队、低频发布等待及封装，不能把它缩称纯序列化时间。最后一段包括Reader盖戳/发送、TCP缓冲与传输、消费者唤醒及完整记录接收，不称为纯物理线路延迟。页面不新增解码、控制、射线、绘制或显示扫描耗时；已有底部一秒统计保持。

## 字段与时钟

全部使用已有冻结协议，无Reader升级或新字段：

- PUBG Consumer v3：ConsumerCapture.started_qpc / read_finished_qpc；Baseline字段9、Update字段15的send_started_qpc。使用SDK已确认binding.clock的FromQpc转换到同一来源纳秒域，接收使用Session记录的本机完整记录QPC，不借用投影/回调执行时刻。
- CS2 Reader v1：CaptureInterval.started_qpc / read_finished_qpc；Envelope.send_started_qpc。消费已有解码字段，按本批来源frequency归一化；接收使用Socket.poll返回的完整记录QPC。
- 两游戏的finished_qpc都是CPU整理/提交完成，不能拿它代替read_finished_qpc。
- 前两段为同一Reader时钟相减，无需消费端校准。第三段通过各连接原有SourceClockEstimate对齐，显示其估算不确定度；缺少、过期、非法校准或负的映射区间时不填0、不用RTT/2代替实测区间。

## 代码边界

`domain/reader_chain_metrics`只消费标准化时间、来源身份及原有校准，不依赖游戏、网络、窗口或GPU。每连接实例由对应遥测锁保护，最多2000个毫秒桶，帧数不设采样上限。计时窗口以完整接收时刻入桶；重复capture、心跳、校准刷新、重绘不增加样本，不对未收到的采集帧补零。

PUBG适配器通过SourceState携带只读时间输入，由既有ReaderMetrics的各通道单独聚合；CS2适配器在真实完整快照接收分支聚合。新连接、来源实例/时钟域/epoch变化清空；校准版本更新不伪造新帧。各阶段独立计算有效样本数，缺读完字段不遮蔽仍可独立测得的传输段。过两秒旧样本自然到期，断连/撤销清空。

`application/diagnostics/reader_chain`只序列化有界汇总；shared ReaderMetrics各channel新增可选chain DTO，windowMs固定2000，与已有windowMs=1000的流量/FPS/GPU统计分开。UI共用ReaderChain组件放在Reader页面，不放在各游戏SourcePanel或数据看板，不逐帧向JS发送记录。没有引入新的计时采样线程、连接、限帧或输出资格。

## 验证

原生测试覆盖三个准确端点、两秒边界、逐帧平均、真零/缺失/反序、未来发送、时钟缺失/过期、重复帧、来源epoch、连接隔离和高频同桶计数。真实CS2 TCP夹具保留大于2^53的原始固定64位锚点，核对读4ms、准备296ms、无校准的传输段为空，以及撤销清空。UI检查每订阅三列、GPU不可用时仍显示、不同阶段缺失、断开/旧DTO、无后续绘制数值及中英文。具体证据见PROJECT_MEMORY最新条目。
