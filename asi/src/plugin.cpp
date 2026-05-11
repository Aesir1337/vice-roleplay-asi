#include "viceasi/plugin.hpp"
#include "viceasi/hooks.hpp"

#include "RakHook/rakhook.hpp"

namespace vice::plugin {

void install() {
    if (!rakhook::samp_addr() || rakhook::samp_version() == rakhook::samp_ver::unknown)
        return;

    hooks::install();
}

void shutdown() {
    hooks::uninstall();
    rakhook::destroy();
}

} // namespace vice::plugin
