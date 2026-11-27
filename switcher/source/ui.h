/*  Talonflame Switcher — minimal OSScreen text UI + GamePad input
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

#include <cstdint>
#include <string>

namespace ui {

bool init();
void shutdown();

void begin();                                   // clear both screens
void line(int row, const std::string &text);    // draw on TV + GamePad
void end();                                     // flip both screens

// GamePad buttons newly pressed this frame (VPAD_BUTTON_* bitmask).
uint32_t pressed();

constexpr int ROWS = 17;  // usable rows on the GamePad
constexpr int COLS = 66;  // usable columns on the GamePad

} // namespace ui
