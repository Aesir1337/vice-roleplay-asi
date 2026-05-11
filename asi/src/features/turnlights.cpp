// vice::features::turnlights — universal turn-signal lights for all vehicles.
// Originally by Den_spb (plugin-sdk samples), adapted into the modular layout.

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

using namespace plugin;

namespace vice::features::turnlights {

namespace {

constexpr unsigned int kBlinkPeriodMs = 500;
constexpr float        kVisibilityRadius = 200.0f;

enum class light_state : unsigned char {
    off, left, right, both
};

class state_store {
public:
    state_store(CVehicle *) : value(light_state::off) {}
    light_state value;
};

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

light_state choose_for_player(CVehicle *) {
    if (KeyPressed('Z'))           return light_state::left;
    if (KeyPressed('X'))           return light_state::both;
    if (KeyPressed('C'))           return light_state::right;
    if (KeyPressed(VK_SHIFT))      return light_state::off;
    return light_state::off; // caller keeps previous value when "no change"
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

VehicleExtendedData<state_store> g_store;

void on_vehicle_render(CVehicle *v) {
    if (!v || !CPools::ms_pVehiclePool)
        return;
    if (!eligible(v)) return;

    auto &state = g_store.Get(v).value;
    if (v->m_pDriver) {
        CPed *playa = FindPlayerPed();
        const bool player_drives = playa && playa->m_pVehicle == v && playa->bInVehicle;
        if (player_drives) {
            const auto picked = choose_for_player(v);
            // Player input: any keypress wins; otherwise keep previous state.
            if (picked != light_state::off || KeyPressed(VK_SHIFT))
                state = picked;
        } else {
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

class installer {
public:
    installer() {
        Events::vehicleRenderEvent.before += on_vehicle_render;
    }
} g_installer;

} // namespace

} // namespace vice::features::turnlights
