# 原生工程与离线验证

正式解决方案 `mousebox-aimbot.sln` 只包含 `mousebox-core` 静态库和 `mousebox-native` 可执行程序。本地解决方案额外包含 `mousebox-tests`。日常增量构建、改动验证和发布入口见[工作流](BUILD_WORKFLOW.md)。界面工作区由pnpm管理。

## 构建

在仓库根目录使用 PowerShell：

```powershell
./tools/build.ps1 -Configuration Debug -IncludeLocalTools
./tools/build.ps1 -Configuration Release -IncludeLocalTools
```

省略 `-IncludeLocalTools` 只构建正式两个项目。输出位于 `out/x64/<Configuration>`。默认增量编译、不清理对象文件，编译并行度默认4，可用`-CompilerProcesses`调整。VS直接构建也在core统一核验冻结Reader/CS2契约和公开二进制边界，不由多个入口重复执行。自有代码采用C++20、严格警告；上游消费源码和Ruckig保留独立警告策略。对象文件按`src`、`vendor-reader`、`vendor-ruckig`和来源子目录隔离，避免同名文件冲突。

## 测试入口

```powershell
./tools/test.ps1 -Configuration Release
./tools/test.ps1 -Configuration Debug -Suite application -SkipBuild
./out/x64/Release/mousebox-tests.exe --suite window
```

`all` 运行 core、reader、render、application；默认不创建可见窗口，不采集真实输入或连接设备。各 suite 保留原有算法、时间、CRC/STOP、目标资格、Reader、速度模型与存储检查。官方 Protobuf 互验所需固定夹具的统一测试入口是 `mousebox-tests --suite reader --export <目录>`。

`window` 是显式原型验证：只创建不可见的 Win32 父子窗口，检查句柄归属、边界、嵌入、WARP/自动显卡、自己的 GPU 缓冲区、最新场景合并、尺寸恢复、设备恢复和销毁；不显示或激活窗口，不注册原始输入，不操作桌面。它不证明 Electron 跨进程、跨屏 DPI、焦点、鼠标回报率或游戏效果。

## Reader 与 CLI

```powershell
./out/x64/Release/mousebox-native.exe --help
./out/x64/Release/mousebox-native.exe controller --help
./out/x64/Release/mousebox-native.exe reader --help
./out/x64/Release/mousebox-native.exe replay --help
```

`controller` 保留原有串口诊断；`reader`（别名 `reader-client`）只读观察；`replay` 只离线重建。后两者在分派 CLI 后不会创建桌面运行服务或串口输出。重放默认读取 `Living-2` 和 `Local-1`，仍兼容 `--living`、`--view`、`--output` 参数。

`tests/reader-test-requirements.txt` 固定官方 Protobuf 运行库。`tools/test-reader-loopback.ps1` 使用调用者指定的 Python 执行五种本机 TCP 夹具；测试不会联系远程 Reader。回环保留 wire/state 证据，可供 `replay` 验证；缺坐标的夹具会如实输出 `CoordinatesMissing`，不授予运动资格。

## 合成压力入口

```powershell
./out/x64/Release/mousebox-tests.exe --suite benchmark --seconds 1800 --hz 144 --entities 200 --output out/native-benchmark.json
```

它使用不可见的原生 DirectX 子窗口，按指定速率生成每人 20 骨点的独立不可变场景。`--hz` 只控制合成输入频率，不限制绘制帧率。记录投影、绘制、提交及提交到观测到绘制的 P95/P99，最新版本、合并数、空闲重绘和每 10 秒进程内存。分位数保留最近 8192 个观测样本，最大值与计数覆盖全程；观测绘制延迟包含最多一个合成输入轮询间隔。

报告每 10 秒原子更新，只有完整运行并验证末场景与空闲后才写 `complete: true`。初始设备/驱动分配要与稳定阶段分开判断；本入口不代表 Electron 全链路压力，不提供迁移前基线，不能据此宣称 10% 回退指标通过。长测应复制测试 EXE 至独立输出目录运行，避免占用正在构建的测试文件。
