#ifndef VICEASI_HOOKS_HPP
#define VICEASI_HOOKS_HPP

#include <windows.h>

namespace vice::hooks {

using game_loop_fn = void (*)();
using wndproc_fn   = LRESULT(CALLBACK *)(HWND, UINT, WPARAM, LPARAM);

void install();
void uninstall();

} // namespace vice::hooks

#endif // VICEASI_HOOKS_HPP
