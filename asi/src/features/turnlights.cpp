// vice::features::turnlights — universal turn-signal lights with SA:MP 0.3DL sync.
// Local rendering originally adapted from plugin-sdk samples (Den_spb, DK22Pac).
// Network sync hijacks the trailing bits of outgoing InCarSync (ID 200) and
// reads them back on the receive side — no server cooperation needed.

#include "plugin.h"
#include "CVehicle.h"
#include "common.h"
#include "CModelInfo.h"
#include "CTimer.h"
#include "CCoronas.h"
#include "CGeneral.h"
#include "CCamera.h"
#include "CPathFind.h"
#include "CPools.h"
#include "ePedType.h"

#include "RakHook/rakhook.hpp"
#include "RakNet/BitStream.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <optional>

using namespace plugin;

namespace vice::features::turnlights {

namespace {

// ---- rendering knobs ------------------------------------------------------
constexpr unsigned int kBlinkPeriodMs    = 500;
constexpr float        kVisibilityRadius = 200.0f;

// ---- wire-format trailer --------------------------------------------------
// Outgoing InCarSync (ID 200) gets `[0xAB, state_byte]` appended at the end.
// 0xAB is just a magic so we never mis-decode a vanilla packet (no false
// positives from clients without the asi). The server forwards the bitstream
// to peers; the trailer rides along intact in practice on 0.3DL.
constexpr unsigned char kIncarSyncId  = 200;
constexpr unsigned char kTrailerMagic = 0xAB;

enum class light_state : unsigned char {
    off = 0, left = 1, right = 2, both = 3
};

class state_store {
public:
    state_store(CVehicle *) : value(light_state::off) {}
    light_state value;
};

// ---- local + remote state -------------------------------------------------
std::atomic<unsigned char> g_local_state{static_cast<unsigned char>(light_state::off)};

struct remote_entry {
    DWORD       last_seen_ms = 0;
    light_state state        = light_state::off;
};
// SA:MP 0.3DL caps the player pool at 1004; one slot per id is cheap.
std::array<remote_entry, 1005> g_remote{};

constexpr DWORD kRemoteTtlMs = 1200;

// ---- shared helpers (rendering side) --------------------------------------
CVector2D path_link_position(CCarPathLinkAddress &address) {
    if (address.m_nAreaId != -1 && address.m_nCarPathLinkId != -1 && ThePaths.m_pPathNodes[address.m_nAreaId]) {
        const auto &node = ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId];
        return CVector2D(static_cast<float>(node.m_vecPosn.x) / 8.0f,
                         static_cast<float>(node.m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

float z_angle_for(CVector2D const &p) {
    float a = CGeneral::GetATanOfXY(p.x, p.y) * 57.295776f - 90.0f;
    while (a < 0.0f) a += 360.0f;
    return a;
}

void draw_corona(CVehicle *v, unsigned dummy_id, bool left) {
    CVector posn = reinterpret_cast<CVehicleModelInfo *>(
                       CModelInfo::ms_modelInfoPtrs[v->m_nModelIndex])
                       ->m_pVehicleStruct->m_avDummyPos[dummy_id];
    if (posn.x == 0.0f) posn.x = 0.15f;
    if (left) posn.x *= -1.0f;

    CCoronas::RegisterCorona(
        reinterpret_cast<unsigned int>(v) + 50 + dummy_id + (left ? 0 : 2),
        v, 255, 128, 0, 255, posn,
        0.3f, 150.0f, CORONATYPE_SHINYSTAR, eCoronaFlareType::FLARETYPE_NONE,
        false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
}

void draw_for(CVehicle *v, light_state s) {
    if (s == light_state::both || s == light_state::right) {
        draw_corona(v, 0, false);
        draw_corona(v, 1, false);
    }
    if (s == light_state::both || s == light_state::left) {
        draw_corona(v, 0, true);
        draw_corona(v, 1, true);
    }
}

bool eligible(CVehicle *v) {
    if (v->m_nVehicleSubClass != VEHICLE_AUTOMOBILE && v->m_nVehicleSubClass != VEHICLE_BIKE)
        return false;
    const auto a = v->GetVehicleAppearance();
    if (a != VEHICLE_APPEARANCE_AUTOMOBILE && a != VEHICLE_APPEARANCE_BIKE)
        return false;
    return v->bEngineOn && v->m_fHealth > 0 && !v->bIsDrowning && !v->m_pAttachedTo;
}

light_state choose_for_player() {
    if (KeyPressed('Z'))      return light_state::left;
    if (KeyPressed('X'))      return light_state::both;
    if (KeyPressed('C'))      return light_state::right;
    if (KeyPressed(VK_SHIFT)) return light_state::off;
    return light_state::off; // caller decides whether to apply.
}

light_state choose_for_ai(CVehicle *v) {
    CVector2D prev = path_link_position(v->m_autoPilot.m_nPreviousPathNodeInfo);
    CVector2D cur  = path_link_position(v->m_autoPilot.m_nCurrentPathNodeInfo);
    CVector2D nxt  = path_link_position(v->m_autoPilot.m_nNextPathNodeInfo);

    float angle = z_angle_for(nxt - cur) - z_angle_for(cur - prev);
    while (angle < 0.0f)   angle += 360.0f;
    while (angle > 360.0f) angle -= 360.0f;

    if (angle >= 30.0f && angle < 180.0f) return light_state::left;
    if (angle <= 330.0f && angle > 180.0f) return light_state::right;

    if (v->m_autoPilot.m_nCurrentLane == 0 && v->m_autoPilot.m_nNextLane == 1)
        return light_state::right;
    if (v->m_autoPilot.m_nCurrentLane == 1 && v->m_autoPilot.m_nNextLane == 0)
        return light_state::left;

    return light_state::off;
}

bool blink_phase_on() {
    return CTimer::m_snTimeInMilliseconds % (kBlinkPeriodMs * 2) < kBlinkPeriodMs;
}

// Returns the freshest (newest, within TTL) remote-sync entry, including
// `off`. Empty optional means "no recent sync from anyone" — caller decides
// whether to fall back further. Single-source attribution: good enough while
// at most one remote driver is visible. Multi-driver attribution needs the
// SA:MP CPlayerPool offsets for 0.3DL — planned for a follow-up.
std::optional<light_state> freshest_remote_within_ttl() {
    const DWORD now      = GetTickCount();
    DWORD       best_age = kRemoteTtlMs;
    std::optional<light_state> best;
    for (const auto &e : g_remote) {
        if (e.last_seen_ms == 0) continue;
        const DWORD age = now - e.last_seen_ms;
        if (age < best_age) {
            best_age = age;
            best     = e.state;
        }
    }
    return best;
}

VehicleExtendedData<state_store> g_store;

void on_vehicle_render(CVehicle *v) {
    if (!v || !CPools::ms_pVehiclePool) return;
    if (!eligible(v))                    return;

    auto &state = g_store.Get(v).value;

    if (v->m_pDriver) {
        CPed      *playa         = FindPlayerPed();
        const bool player_drives = playa && playa->m_pVehicle == v && playa->bInVehicle;
        if (player_drives) {
            const auto picked = choose_for_player();
            if (picked != light_state::off || KeyPressed(VK_SHIFT))
                state = picked;
            g_local_state.store(static_cast<unsigned char>(state), std::memory_order_relaxed);
        } else if (v->m_pDriver->m_nPedType <= PED_TYPE_PLAYER_UNUSED) {
            // Another player: trust the wire-synced state — even when it's
            // OFF. The path-node AI heuristic doesn't apply to remote players
            // (their m_autoPilot fields aren't populated → garbage in, lefts
            // out).
            const auto fresh = freshest_remote_within_ttl();
            state = fresh.value_or(light_state::off);
        } else {
            // Real game AI driver — original path-node heuristic stays useful.
            state = choose_for_ai(v);
        }
    }

    if (!blink_phase_on()) return;
    if ((TheCamera.m_vecGameCamPos - v->GetPosition()).Magnitude() >= kVisibilityRadius)
        return;

    draw_for(v, state);
    if (v->m_pTractor)
        draw_for(v->m_pTractor, state);
}

// ---- network: outgoing InCarSync ----------------------------------------
bool on_send_packet(RakNet::BitStream *bs, PacketPriority &, PacketReliability &, char &) {
    if (!bs || bs->GetNumberOfBytesUsed() < 1)            return true;
    if (bs->GetData()[0] != kIncarSyncId)                 return true;

    bs->SetWriteOffset(static_cast<int>(bs->GetNumberOfBitsUsed()));
    bs->Write<unsigned char>(kTrailerMagic);
    bs->Write<unsigned char>(g_local_state.load(std::memory_order_relaxed));
    return true;
}

bool on_receive_packet(Packet *p) {
    if (!p || p->length < 4)                              return true;
    if (p->data[0] != kIncarSyncId)                       return true;
    if (p->data[p->length - 2] != kTrailerMagic)          return true;

    const unsigned char raw = p->data[p->length - 1];
    if (raw > static_cast<unsigned char>(light_state::both)) return true;

    // Relayed InCarSync layout from server: [id][senderId_word][...sync...]
    const unsigned short sender_id = *reinterpret_cast<const unsigned short *>(p->data + 1);
    if (sender_id >= g_remote.size())                     return true;

    g_remote[sender_id].state        = static_cast<light_state>(raw);
    g_remote[sender_id].last_seen_ms = GetTickCount();
    return true;
}

class installer {
public:
    installer() {
        Events::vehicleRenderEvent.before += on_vehicle_render;
        rakhook::on_send_packet           += on_send_packet;
        rakhook::on_receive_packet        += on_receive_packet;
    }
} g_installer;

} // namespace

} // namespace vice::features::turnlights
