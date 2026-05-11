#include <windows.h>

#include "viceasi/plugin.hpp"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        vice::plugin::install();
        break;
    case DLL_PROCESS_DETACH:
        vice::plugin::shutdown();
        break;
    default:
        break;
    }
    return TRUE;
}
