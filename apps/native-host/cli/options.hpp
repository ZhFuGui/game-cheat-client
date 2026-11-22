#pragma once
#include <cstdint>
#include <string>

struct Options {
    std::wstring port;
    int key = 0x77; // F8
    std::wstring keyName = L"F8";
    std::int16_t dx = 1, dy = 0;
    std::uint16_t interval = 10, budget = 32;
    bool list = false, help = false, probe = false;
};
Options parseOptions(int argc, wchar_t** argv);
void printHelp();
