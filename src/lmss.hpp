/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include "context.hpp"
#include "display.hpp"
#include "event_loop.hpp"
#include "usb.hpp"


class lmss final : public context {
public:
    lmss(logger &);

    event_loop & get_el() override { return el; }
    void set_mouse_pos(mouse_pos_t const &) override;
    void mouse_at_border(mouse_pos_t const &) override;

    void run() { el.run(); }

private:
    void heartbeat(int);

    logger & log;
    event_loop el;
    usb_dev usb;
    display dsp;
    file_descriptor tfd;
};
