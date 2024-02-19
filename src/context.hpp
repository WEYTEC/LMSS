/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include "event_loop.hpp"
#include "types.hpp"

class context {
public:
    virtual event_loop & get_el() = 0;
    virtual void set_mouse_pos(mouse_pos_t const &) = 0;
    virtual void mouse_at_border(mouse_pos_t const &) = 0;
};
