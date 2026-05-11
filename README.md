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
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Output: `build/asi/Release/vice-roleplay.asi` — drop it into your GTA SA install folder.

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
