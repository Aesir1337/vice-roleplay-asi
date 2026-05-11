#include "viceasi/plugin.hpp"
#include "viceasi/hooks.hpp"

#include "RakHook/rakhook.hpp"

#include <windows.h>

namespace vice::plugin {

namespace {

constexpr auto kRequiredVersion = rakhook::samp_ver::v03dlr1;

void fail_version() {
    MessageBoxA(nullptr,
                "vice-roleplay\n\n"
                "VCG:1000  -  SA:MP version incompatible.\n"
                "Yalnizca SA:MP 0.3DL-R1 desteklenmektedir.\n\n"
                "Lutfen SA:MP 0.3DL-R1 kurulumu uzerinden baglanin.",
                "vice-roleplay  -  VCG:1000",
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    TerminateProcess(GetCurrentProcess(), 0xC0DE1000);
}

} // namespace

void install() {
    if (!rakhook::samp_addr())
        return;
    if (rakhook::samp_version() != kRequiredVersion) {
        fail_version();
        return;
    }

    hooks::install();
}

void shutdown() {
    hooks::uninstall();
    rakhook::destroy();
}

} // namespace vice::plugin
