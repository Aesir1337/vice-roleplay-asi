#include "viceasi/game.hpp"

#ifdef VICEASI_HAS_PLUGIN_SDK
#include <plugin.h>
#include "CMessages.h"
#include "CWorld.h"
#include "CPlayerInfo.h"
#include "CPlayerPed.h"
#include "common.h"

#include <array>
#include <cstring>
#endif

namespace vice::game {

#ifdef VICEASI_HAS_PLUGIN_SDK

namespace {
char *copy_to_static(std::string_view text) {
    static std::array<char, 256> buf{};
    const size_t n = (text.size() < buf.size() - 1) ? text.size() : buf.size() - 1;
    std::memcpy(buf.data(), text.data(), n);
    buf[n] = '\0';
    return buf.data();
}
} // namespace

bool player_active() {
    return FindPlayerPed() != nullptr;
}

void player_position(float &x, float &y, float &z) {
    const auto v = FindPlayerCoors();
    x = v.x; y = v.y; z = v.z;
}

int player_money() {
    if (!CWorld::Players)
        return 0;
    return CWorld::Players[0].m_nMoney;
}

void hud_message(std::string_view text, int duration_ms) {
    CMessages::AddMessageJumpQ(copy_to_static(text), static_cast<unsigned int>(duration_ms), 0, true);
}

void big_message(std::string_view text, int style) {
    CMessages::AddBigMessageQ(copy_to_static(text), 4000, static_cast<unsigned short>(style));
}

#else // !VICEASI_HAS_PLUGIN_SDK

bool player_active() { return false; }
void player_position(float &x, float &y, float &z) { x = y = z = 0.0f; }
int  player_money() { return 0; }
void hud_message(std::string_view, int) {}
void big_message(std::string_view, int) {}

#endif

} // namespace vice::game
