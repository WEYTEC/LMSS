/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include <cstdint>


enum border_t : uint8_t {
    TOP = 0,
    BOTTOM,
    LEFT,
    RIGHT,
    HIDE
};

struct mouse_pos_t {
    uint8_t screen;
    uint8_t border;
    uint16_t pos;
};
