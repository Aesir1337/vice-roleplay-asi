# vice-roleplay-asi — Proje Kuralları

Bu dosya, projede çalışırken uyulması gereken kuralları tanımlar. Yeni kod, refactor veya bağımlılık ekleme işlemlerinden önce mutlaka okunmalıdır.

---

## 1. Mimari Sınırlar

Proje iki ayrı katmandan oluşur — sınırları geçmek **yasak**:

| Katman | Yol | Durum |
|---|---|---|
| RakHook kütüphanesi | `include/RakHook/`, `src/RakHook/` | Üst-akış kütüphane. Sadece bug-fix için dokunulur. Yeni özellik buraya yazılmaz. |
| RakNet kaynak kodu | `include/RakNet/`, `src/RakNet/` | Üçüncü taraf. **Asla** düzenlenmez. Warning bile burada bastırılmaz — gerekirse consumer tarafında `target_include_directories(... SYSTEM ...)` kullanılır. |
| ASI plugin | `asi/` | Tüm yeni iş burada yapılır. |

**Kural:** Tüm uygulama davranışı (hooks, action, event handler, UI vb.) `asi/` altında yaşar. `src/RakHook` veya `src/RakNet` içine `vice::*` referansı eklenmez (bağımlılık tek yönlü: `asi/` → `RakHook`).

---

## 2. Modül Yapısı (`asi/`)

```
asi/
  CMakeLists.txt           target: vice-roleplay (.asi)
  include/viceasi/         PUBLIC API headerları
    plugin.hpp             vice::plugin::install/shutdown
    hooks.hpp              vice::hooks::install/uninstall
    actions.hpp            vice::actions::*
    game.hpp               vice::game::* (GTA SA helpers)
    bitstream_utils.hpp    vice::utils::*
  src/features/            izole oyun-özelliği modülleri (vice::features::*)
  src/
    dllmain.cpp            DllMain — sadece plugin::install/shutdown çağırır
    plugin.cpp             Bootstrap: samp_version kontrolü → hooks::install
    hooks.cpp              game_loop + wndproc hook'ları
    actions.cpp            Kullanıcı tetiklemeli aksiyonlar
```

**Namespace haritası:**

| Namespace | Sorumluluk |
|---|---|
| `vice::plugin` | Üst-seviye lifecycle (install/shutdown) |
| `vice::hooks` | SA:MP/oyun fonksiyonu hook'ları (polyhook detour) |
| `vice::actions` | Kullanıcı/hotkey ile tetiklenen rakhook RPC/packet emülasyonları |
| `vice::utils` | Tipten bağımsız küçük helper'lar (bitstream okuma/yazma vb.) |

### Yeni modül eklerken
1. Yeni header: `asi/include/viceasi/<isim>.hpp` — yeni bir `vice::<isim>` namespace açar.
2. Impl: `asi/src/<isim>.cpp`.
3. `asi/CMakeLists.txt` içindeki `target_sources(vice-roleplay PRIVATE ...)` listesine ekle.
4. Modüller arası bağımlılık tek yönlü olmalı: `dllmain` → `plugin` → `hooks`/`actions` → `utils`. Aşağıdan yukarı include **yasak**.

---

## 3. Kod Stili

### Header guard
`#ifndef VICEASI_<MODULE>_HPP` (örn. `VICEASI_HOOKS_HPP`). `#pragma once` kullanılmaz — mevcut header'larla tutarlılık.

### İsimlendirme
- Namespace, fonksiyon, değişken: `snake_case`
- Tip alias: `snake_case_t` (örn. `game_loop_fn`) — eski rakhook stiline uyumlu, ama tercihen `_fn` suffix'i.
- Sabit: `kPascalCase` (örn. `kGameLoopAddr`)
- Üye değişken: `g_` prefix'i sadece dosya-static singleton'lar için (`g_game_loop_hook`).

### Format
- `.clang-format` mevcut; commit öncesi `clang-format -i` çalıştır.
- Satır 140 char'a kadar tolere edilir, hedef 120.

### Yorum
Default: **yorum yazma.** Sadece şu durumlarda:
- Bir adresin/offset'in neden literal olduğu (örn. SA:MP RPC ID).
- Bir warning suppression'un nedeni (örn. `/wd4717` için).

`// removed`, `// TODO unused`, kişi/PR referansı içeren yorumlar yasak.

### Debug / log yasağı
ASI release bir wrapper'dır — kullanıcının görmesi gereken çıktı **yoktur**.

| Yasak | Neden |
|---|---|
| `std::cout`, `printf`, `OutputDebugString` | Release binary'de debug noise olur |
| `MessageBox` | UX bozar, SAMP içinde modal gerektirir |
| `[HOOKED]`, `[DEBUG]` gibi görünür string injection | Demo davranışıdır, release'de olmaz |
| `#ifdef _DEBUG` ile sadece debug build'de aktif log | Build varyantına göre davranış sapması yaratır |

Debug ihtiyacı varsa: yerel branch'te geçici, **commit'lenmez**.

### Hooking pattern
Yeni bir oyun fonksiyonu hook'lamak için `asi/src/hooks.cpp` içindeki mevcut pattern'i kullan:

```cpp
constexpr std::uintptr_t kTargetAddr = 0x123456;
std::unique_ptr<cyanide::polyhook_x86<target_fn, decltype(&on_target)>> g_target_hook;

void on_target(target_fn orig, /* args */) {
    // pre
    orig(/* args */);
    // post
}
```

**Kural:** Hook fonksiyonu kayıt etmek `install()` içinde; serbest bırakmak `uninstall()` içinde. Hook ömrü plugin ömrüne bağlı.

---

## 4. Build Kuralları

### Hedef
- Generator: `Visual Studio 17 2022`, platform: **`Win32` (zorunlu)** — `static_assert(sizeof(size_t) == 4)` enforce eder.
- Config: `Release` (deploy), `Debug` (yerel iterasyon).
- CRT: `MultiThreaded$<$<CONFIG:Debug>:Debug>` (statik) — `.asi` GTA process'inin CRT'sine bağlı kalmaz.
- C++ standart: `cxx_std_20` (rakhook PUBLIC).
- Warning seviye: `/W3 /permissive-`.

### Build komutu
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Release
```
Çıktı: `build/asi/Release/vice-roleplay.asi`.

`-DCMAKE_POLICY_VERSION_MINIMUM=3.5` flag'i polyhook2 alt-bağımlılıklarının (xbyak, zydis) CMake 4.x ile çakışmasını çözer — kaldırma, bağımlılıklar güncellenmedikçe gereklidir.

### Warning policy
- **Kendi kodumuzda (asi + RakHook) 0 warning hedeflenir.** Yeni eklenen warning commit'i blocker'dır.
- 3rd party kütüphanelerden (polyhook2 vb.) gelen warning'ler `target_compile_options(<3rd_party_target> PRIVATE /wd<code>)` ile **kaynağında** bastırılır, projenin genel warning seviyesinden düşürerek değil.
- Mevcut bastırma: `PolyHook_2` üzerinde `/wd4717` (bkz. `src/CMakeLists.txt`).

### Build doğrulama
1. `cmake --build build --config Release` → 0 warning, 0 error.
2. `dumpbin /headers build\asi\Release\vice-roleplay.asi | findstr machine` → `14C machine (x86)`.
3. Grep: `HOOKED|std::cout|printf|MessageBox|OutputDebug` proje dışı `RakNet/` hariç boş dönmeli.

---

## 5. Bağımlılıklar

Yeni bağımlılık eklenmeden önce:
- FetchContent ile statik link edilmeli (DLL bağımlılığı yok).
- 32-bit derlenebilmeli.
- Statik CRT (`/MT`) ile çakışmamalı.
- License GPL/MIT/BSD/zlib uyumlu olmalı.

Mevcut bağımlılıklar (`src/CMakeLists.txt` üzerinden):
- `cyanide` (hook abstraction)
- `polyhook2` + asmjit + asmtk + zydis (detour engine)

Opsiyonel (varsayılan ON, `cmake/plugin-sdk.cmake`):
- `DK22Pac/plugin-sdk` (Zlib) — GTA SA sınıfları & event sistemi.
  - **Lib build'i tam değil:** sadece bir avuç shared + game_sa cpp'si (Pattern, PluginBase, CMessages, CWorld, CVehicle, vb.) `plugin_sdk_core` static lib'i içinde compile edilir. Geri kalan header'lar inline çağrılarla GTA SA adreslerine bağlanır — `vice::features::*` modülleri eklerken yeni link hatası çıkarsa `cmake/plugin-sdk.cmake` `PLUGIN_SDK_CORE_SOURCES` listesine ilgili `.cpp`'yi ekle.
  - **RenderWare SDK gerekir.** `RWGSDK` env var, `RWGDIR` env var, veya `-DVICEASI_RW_SDK_ROOT=<path>` ile bulunur. plugin-sdk RW define edilmiş şekilde derlenir; RW header yolu (varsayılan `d3d9`) `VICEASI_RW_PLATFORM` ile değiştirilebilir.
  - `vice::features::*` kodu plugin-sdk sınıflarını **`.cpp` içinde** kullanabilir; public header'larda (`asi/include/viceasi/*.hpp`) plugin-sdk include'u **yasak** — wrapper API saf C++ kalır.

---

## 6. Git Akışı

### Branch
- `main` — her zaman build alabilir durumda. Doğrudan commit yapılmaz, feature branch üzerinden merge edilir.
- `feature/<konu>`, `fix/<konu>` — kısa ömürlü iş branch'leri.

### Commit mesajı
```
<type>: <kısa özet, imperatif, ≤72 char>

<opsiyonel gövde — neden, dikkat gerektiren detaylar>
```
`type`: `release`, `feat`, `fix`, `refactor`, `chore`, `docs`, `build`.

Örnek:
```
feat: add chat command emulator hotkey

NUMPAD7 binding triggers vice::actions::emul_chat_command via rakhook.
```

### Push
- `main`'e force push **yasak** (initial setup hariç — onaylı).
- PR yokken bile self-review: `git diff origin/main...HEAD` çıktısını oku.

---

## 7. Hızlı Kontrol Listesi (commit öncesi)

- [ ] `cmake --build build --config Release` 0 warning, 0 error.
- [ ] Yeni dosyalar `asi/CMakeLists.txt`'ye eklendi.
- [ ] Hiçbir `std::cout` / `[HOOKED]` / `MessageBox` eklenmedi.
- [ ] Header guard `VICEASI_*_HPP` formatında.
- [ ] Yeni public API `vice::` namespace'i altında.
- [ ] `src/RakHook` veya `src/RakNet` dokunulmamış (gerekli istisna ise neden commit mesajında).
- [ ] `clang-format` uygulandı.
