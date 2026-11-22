#include "platform/windows/console.hpp"
#include <Windows.h>
#include <clocale>

namespace mousebox {
void initializeConsole() {
    std::setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);
}
}
