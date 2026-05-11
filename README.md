# vice-roleplay-asi

A clean SA:MP `.asi` plugin built on top of the [RakHook](https://github.com/imring/RakHook) library.
Supports SA:MP versions `0.3.7-R1`, `0.3.7-R3-1`, `0.3.7-R4` and `0.3DL-R1`.

The repository ships both:
- The **RakHook** static library (under `include/` and `src/`).
- The **vice-roleplay.asi** plugin (under `asi/`), a modular wrapper that loads RakHook into SA:MP.

## Project layout

```
include/        public RakHook + RakNet headers
src/            RakHook + RakNet sources (static library target: rakhook)
asi/            vice-roleplay.asi plugin
  include/viceasi/   plugin public headers (plugin / hooks / actions / utils)
  src/               dllmain, plugin bootstrap, hooks, demo actions
```

## Build

Requires **Visual Studio 2022** (or Build Tools) and **CMake 3.20+**.
The plugin is 32-bit only (SA:MP is a 32-bit process).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 `
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 `
      -DVICEASI_RW_SDK_ROOT="C:/RW/Graphics/rwsdk"
cmake --build build --config Release
```

Output: `build/asi/Release/vice-roleplay.asi` — drop it into your GTA SA install folder.

### Dependencies
- **RenderWare SDK** (required when `VICEASI_USE_PLUGIN_SDK=ON`, the default).
  Resolved from `-DVICEASI_RW_SDK_ROOT`, `$env:RWGSDK`, or `$env:RWGDIR\rwsdk`.
  Platform variant via `-DVICEASI_RW_PLATFORM=d3d9` (default; `d3d8`/`null`/`opengl` also accepted).
- All other deps (RakHook, cyanide, polyhook2, plugin-sdk) are fetched automatically via CMake `FetchContent`.

### CMake options
| Option | Default | Effect |
|---|---|---|
| `VICEASI_BUILD_ASI` | `ON` | Build the `.asi` plugin |
| `VICEASI_USE_PLUGIN_SDK` | `ON` | Link plugin-sdk for GTA SA helpers (requires RW SDK) |
| `VICEASI_RW_SDK_ROOT` | env-derived | Path to `rwsdk` directory |
| `VICEASI_RW_PLATFORM` | `d3d9` | RW build variant |

## RakHook API

### SA:MP
- `std::uintptr_t rakhook::samp_addr(std::uintptr_t offset = 0)` — get SA:MP module address with an offset.
- `samp_ver rakhook::samp_version()` — get SA:MP version supported by RakHook.

### Events
- `bool rakhook::initialize()` — initialize RakHook.
- `void rakhook::destroy()` — destroy RakHook.
- `on_event<send_t> rakhook::on_send_packet` — outgoing packet.
- `on_event<receive_t> rakhook::on_receive_packet` — incoming packet.
- `on_event<send_rpc_t> rakhook::on_send_rpc` — outgoing RPC.
- `on_event<receive_rpc_t> rakhook::on_receive_rpc` — incoming RPC.

### Send / Emulate
- `bool rakhook::send(RakNet::BitStream *bs, PacketPriority priority, PacketReliability reliability, char ord_channel)`
- `bool rakhook::send_rpc(int id, RakNet::BitStream *bs, PacketPriority priority, PacketReliability reliability, char ord_channel, bool sh_timestamp)`
- `bool rakhook::emul_packet(RakNet::BitStream &pbs)`
- `bool rakhook::emul_rpc(unsigned char id, RakNet::BitStream &rpc_bs)`

## License
See [LICENSE](LICENSE). RakHook © its authors; portions of `hooked_rakclient_interface` are derived from mod_s0beit_sa (GPLv3).
