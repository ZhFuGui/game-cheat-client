#include "options.hpp"
#include <control_wire.h>
#include <iostream>
#include <stdexcept>

namespace
{
int number(const std::wstring &value, int minimum, int maximum)
{
    std::size_t end = 0;
    int parsed = std::stoi(value, &end);
    if (end != value.size() || parsed < minimum || parsed > maximum)
        throw std::invalid_argument("Option value is out of range");
    return parsed;
}
} // namespace
Options parseOptions(int argc, wchar_t **argv)
{
    Options o;
    for (int i = 1; i < argc; ++i)
    {
        std::wstring flag = argv[i];
        if (flag == L"--list")
        {
            o.list = true;
            continue;
        }
        if (flag == L"--help" || flag == L"-h")
        {
            o.help = true;
            continue;
        }
        if (flag == L"--probe")
        {
            o.probe = true;
            continue;
        }
        if (i + 1 >= argc)
            throw std::invalid_argument("Missing option value");
        std::wstring value = argv[++i];
        if (flag == L"--port")
            o.port = value;
        else if (flag == L"--dx")
            o.dx =
                static_cast<std::int16_t>(number(value, -32767, 32767));
        else if (flag == L"--dy")
            o.dy =
                static_cast<std::int16_t>(number(value, -32767, 32767));
        else if (flag == L"--interval")
            o.interval = static_cast<std::uint16_t>(number(value, 2, 50));
        else if (flag == L"--limit")
            o.budget = static_cast<std::uint16_t>(number(value, 1, 65535));
        else if (flag == L"--key")
        {
            if (value.size() < 2 || value[0] != L'F')
                throw std::invalid_argument("Use F1..F12 as the trigger key");
            o.key = 0x70 + number(value.substr(1), 1, 12) - 1;
            o.keyName = value;
        }
        else
            throw std::invalid_argument("Unknown option");
    }
    if (!o.dx && !o.dy && !o.probe)
        throw std::invalid_argument("At least one motion axis must be nonzero");
    return o;
}
void printHelp()
{
    std::wcout << L"鼠标盒子控制工具（TTL，500000 8N1；需要 v6 固件）\n"
                  L"  --list                 列出当前串口及 USB 身份\n"
                  L"  --port COMn            指定 USB-TTL；省略时选择唯一已识别模块\n"
                  L"  --key F8               按住触发，松开停止；支持 F1..F12\n"
                  L"  --dx 1 --dy 0          每步计数，各轴 -100..100\n"
                  L"  --interval 10          步间隔 2..50 ms\n"
                  L"  --limit 32             每次按住的总计数上限 1..200\n"
                  L"  --probe                只下发无运动探测帧，需看板端 CONTROL 诊断确认\n"
                  L"Escape 或 Ctrl+C 退出；达到计数/时间上限后需先松开再按。\n";
}
