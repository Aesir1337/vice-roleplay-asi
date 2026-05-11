#include "viceasi/hooks.hpp"
#include "viceasi/actions.hpp"

#include "RakHook/rakhook.hpp"
#include "RakNet/StringCompressor.h"

#include <cyanide/hook_impl_polyhook.hpp>

#ifdef VICEASI_HAS_PLUGIN_SDK
#include <plugin.h>
#include "Events.h"
#endif

#include <bit>
#include <memory>

namespace vice::hooks {

namespace {

constexpr std::uintptr_t kWndProcAddr = 0x747EB0;

LRESULT CALLBACK on_wndproc(wndproc_fn orig, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

std::unique_ptr<cyanide::polyhook_x86<wndproc_fn, decltype(&on_wndproc)>> g_wndproc_hook;

#ifndef VICEASI_HAS_PLUGIN_SDK
using game_loop_raw_fn = void (*)();
constexpr std::uintptr_t kGameLoopAddr = 0x53BEE0;

void on_game_loop(game_loop_raw_fn orig);

std::unique_ptr<cyanide::polyhook_x86<game_loop_raw_fn, decltype(&on_game_loop)>> g_game_loop_hook;

void on_game_loop(game_loop_raw_fn orig) {
    orig();

    static bool initialized = false;
    if (initialized || !rakhook::initialize())
        return;

    StringCompressor::AddReference();
    initialized = true;
}
#endif

void try_initialize_rakhook() {
    static bool initialized = false;
    if (initialized || !rakhook::initialize())
        return;
    StringCompressor::AddReference();
    initialized = true;
}

LRESULT CALLBACK on_wndproc(wndproc_fn orig, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_KEYUP:
        if (wparam == VK_NUMPAD4)
            actions::change_name();
        break;
    case WM_KEYDOWN:
        if (wparam == VK_NUMPAD5)
            actions::emul_player_sync();
        break;
    default:
        break;
    }
    return orig(hwnd, msg, wparam, lparam);
}

} // namespace

void install() {
#ifdef VICEASI_HAS_PLUGIN_SDK
    static bool subscribed = false;
    if (!subscribed) {
        plugin::Events::initRwEvent += []() { try_initialize_rakhook(); };
        plugin::Events::gameProcessEvent += []() { try_initialize_rakhook(); };
        subscribed = true;
    }
#else
    if (!g_game_loop_hook) {
        g_game_loop_hook = std::make_unique<decltype(g_game_loop_hook)::element_type>(
            std::bit_cast<game_loop_raw_fn>(kGameLoopAddr), &on_game_loop);
        g_game_loop_hook->install();
    }
#endif

    if (!g_wndproc_hook) {
        g_wndproc_hook = std::make_unique<decltype(g_wndproc_hook)::element_type>(
            std::bit_cast<wndproc_fn>(kWndProcAddr), &on_wndproc);
        g_wndproc_hook->install();
    }
}

void uninstall() {
#ifndef VICEASI_HAS_PLUGIN_SDK
    g_game_loop_hook.reset();
#endif
    g_wndproc_hook.reset();
}

} // namespace vice::hooks
