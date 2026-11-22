# Ruckig 本地依赖

上游 https://github.com/pantor/ruckig ，固定 v0.15.3 / `37b6e7a4c60f5befd2506741d5f7d0ae7eefb3db`，MIT，原许可保存在 LICENSE。下载归档 SHA256 `F3B1E084E2343D5A6E4D80FB0054DCEAEAF168235116F792B89D98BB2DA6980B`。

原样保留 include 和 CMake 核心列出的 11 个计算源码；不包含 cloud_client、Python、云端或 Pro Tracking。通过 mousebox-core.vcxproj 静态编译，C++20/MSBuild，第三方源码单独保留自己的警告级别；本项目源码继续启用警告即错误。

PC0.5.5使用本地 state-to-state Position 接口，每步按当前速度/加速度、剩余角度换算的计数和有限目标速度重新计算限 jerk 轨迹。计算单位为1000计数，以保持数值范围；按实际控制时间采样轨迹，保留整数余量。并非完整 Pro Tracking、不保证人类轨迹或 FPS 最优。Reader真实反馈、缺帧暂停、STOP、每步预算和离线验证仍由本项目负责。
