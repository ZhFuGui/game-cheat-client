#include "node_api.hpp"
#include "input/capture_server.hpp"
#include "application/storage/json_store.hpp"
#include <memory>

namespace mousebox::desktop::addon
{
struct Context
{
    std::unique_ptr<CaptureServer> server;
};
void cleanup(napi_env, void *data, void *)
{
    delete static_cast<Context *>(data);
}
napi_value dispatch(napi_env env, napi_callback_info info)
{
    try
    {
        std::size_t count = 2;
        napi_value args[2]{};
        check(api().arguments(env, info, &count, args, nullptr, nullptr));
        if (!count)
            throw std::runtime_error("Native input command missing");
        const auto command = readString(env, args[0]);
        void *instance = nullptr;
        check(api().data(env, &instance));
        auto &context = *static_cast<Context *>(instance);
        application::Json result;
        if (command == "open")
        {
            if (context.server || count != 2)
                throw std::runtime_error("Native input endpoint is already open or options missing");
            const auto options = application::Json::parse(readString(env, args[1]));
            context.server = std::make_unique<CaptureServer>(
                options.at("token").get<std::string>(),
                std::filesystem::path(application::storage::wide(options.at("executable").get<std::string>())));
            result = context.server->inspect();
        }
        else if (command == "inspect")
            result =
                context.server ? context.server->inspect() : application::Json{{"active", false}, {"closed", true}};
        else if (command == "close")
        {
            context.server.reset();
            result = {{"ok", true}};
        }
        else
            throw std::runtime_error("Unknown native input command");
        const auto text = result.dump();
        napi_value value = nullptr;
        check(api().string(env, text.data(), text.size(), &value));
        return value;
    }
    catch (const std::exception &error)
    {
        api().error(env, nullptr, error.what());
        return nullptr;
    }
}
} // namespace mousebox::desktop::addon

NAPI_MODULE_INIT()
{
    using namespace mousebox::desktop::addon;
    try
    {
        auto context = std::make_unique<Context>();
        check(api().setData(env, context.get(), cleanup, nullptr));
        context.release();
        napi_value method = nullptr;
        check(api().function(env, "dispatch", NAPI_AUTO_LENGTH, dispatch, nullptr, &method));
        check(api().property(env, exports, "dispatch", method));
        return exports;
    }
    catch (const std::exception &error)
    {
        // No capture thread or input registration exists before a successful open.
        const auto fail = symbol<decltype(&napi_throw_error)>("napi_throw_error");
        fail(env, nullptr, error.what());
        return nullptr;
    }
}
