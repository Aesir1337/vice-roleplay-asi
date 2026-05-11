#ifndef VICEASI_GAME_HPP
#define VICEASI_GAME_HPP

#include <string_view>

namespace vice::game {

bool player_active();
void player_position(float &x, float &y, float &z);
int  player_money();

void hud_message(std::string_view text, int duration_ms = 4000);
void big_message(std::string_view text, int style = 0);

} // namespace vice::game

#endif // VICEASI_GAME_HPP
