# cs2-external-autowall 公开二进制契约

这里只保留必要的`api.h`与`types.h`，不包含实现、C++包装或上游VS工程。`manifest.json`固定公开头文件、DLL与地图目录的摘要；修改配套版本时使用显式暂存工具并保留旧发布包。

当前产物为 Windows x64 Release 的 cs2-external-autowall.dll，851456 字节，ABI 1，37 个公开导出。
SHA256 为 3b4fa514d88f1674df9f499cd90f919cd960086ab10da36a22c36564c50402f0。
上游仓库 HEAD 为 692c6824587ba6c237283cf9eb7bd7f0baaa6524，采用该工作树已编译并经上游 113 项验证的产物。
api.h/types.h 与上次逐字节一致，按上游新公开目录重新定位。
旧 DLL、清单及上游产物记录位于 .migration/traceline-before-autowall-20261116。

运行资源在`data/games/cs2/cover`：固定DLL、相对路径目录、24张完整静态`.cs2tl` v4缓存和原有武器参数包。本轮没有重建/搬迁地图或更换参数包。缓存含必要原始物理元数据，仍只迁入转换容器，不复制KV3/VPK/内部源码。大型运行文件不进入Git；缺失时构建明确失败，不能自动引用上游开发目录。实际激活状态见PROJECT_MEMORY。旧DLL/清单/头和本轮交付、去甲伤害上界证明保存在`.migration/traceline-before-batch-49a2036-20261027`；更早的整套资源备份保留。

原生静态事件后端保留网格原始朝向、物理属性和原生事件/材料段/损耗语义；公开ABI布局保持兼容，新增`NATIVE_STATIC_EVENTS`标志只表示离线静态来源，不是整枪实战等价声明。缺少数值属性、尚未核验的形状/变换和不确定边界保持回退或未知，不能把24张格式升级等同于全部物理能力齐全。具体接入与验证见[剩余伤害](../../docs/CS2_REMAINING_DAMAGE.md)。预计头伤仍由公开目标伤害API做条件预测，未采集实际命中记录。

`tools/stage-traceline-package.py`只接收显式DLL/公开头目录/地图目录及已审阅DLL摘要；它核对原始清单、复制转换缓存，不复制KV3、VPK或内部源码。`tools/check-traceline-package.ps1 -IncludeScenes`核对完整包；正式发布将这些资源放在原生宿主旁的`data/games/cs2/cover`，运行不依赖原开发路径。

新版整批场景使用`tools/update-traceline-scenes.py`，以明确的最终交付报告和逐文件摘要清单为准。上游早期map-data/manifest.json可能保留旧转换统计，不能直接把它当新版清单。工具逐摘要核验、通过新DLL公开API读取真实场景信息、保留现有参数，验证完毕后把旧完整资源移入回退目录再切换；不删除旧资源。

接口与职责见[游戏扩展](../../docs/GAME_EXTENSIONS.md)。
