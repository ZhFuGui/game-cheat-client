#include "cli/dispatch.hpp"
#include "host/desktop_host.hpp"
#include <iostream>
int wmain(int argc, wchar_t **argv)
{
    try
    {
        if (auto code = native_cli::dispatch(argc, argv))
            return *code;
        return mousebox::desktop::runDesktopHost(argc, argv);
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
