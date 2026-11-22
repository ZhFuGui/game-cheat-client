# Node-API headers

固定为 Node.js v22.9.0 的 `src/node_api.h`、`node_api_types.h`、`js_native_api.h` 和 `js_native_api_types.h`。四份文件已与 [Node.js 上游标签](https://github.com/nodejs/node/tree/v22.9.0/src)逐字节 SHA256 核对；完整上游许可证见本目录 LICENSE。

本项目只使用稳定 Node-API v8。`mousebox-input.node` 通过宿主可执行程序导出的 Node-API 函数装配，不链接 V8、Node 内部 ABI 或特定 Electron 版本的导入库。它只向 Electron 主进程提供端点的打开、检查和关闭；原始鼠标记录及完成数据不经 JavaScript。

构建归属于已有 `mousebox-native.vcxproj` 的附加产物；规则为 `build/input-addon.targets`。不增加正式 VS 工程，不引入内核驱动或驱动安装步骤。
