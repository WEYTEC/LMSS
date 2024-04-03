/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

#include <memory>
#include <string>
#include <vector>

#include "context.hpp"
#include "file_descriptor.hpp"
#include "logger.hpp"
#include "rect.hpp"

class display final {
public:
    display(logger &, context &);
    ~display();

    void set_mouse_pos(mouse_pos_t const &);
    void handle_events(int);

private:
    struct dsp_deleter { void operator()(Display * dsp) { XCloseDisplay(dsp); } };
    using dsp_t = std::unique_ptr<Display, dsp_deleter>;

    struct monitors_deleter { void operator()(XRRMonitorInfo * m) { XRRFreeMonitors(m); } };
    using monitors_t = std::unique_ptr<XRRMonitorInfo, monitors_deleter>;

    struct monitor_t {
        int id;
        int x;
        int y;
        int w;
        int h;
        Window root;

        inline bool operator==(monitor_t const & other) const {
            return std::tie(id, x, y, w, h, root) ==
                std::tie(other.id, other.x, other.y, other.w, other.h, other.root);
        }

        inline bool operator!=(monitor_t const & other) const { return !operator==(other); }
    };

    struct pos_t {
        int x = 0;
        int y = 0;
        Window root = 0;
    };

    monitor_t const & get_mon_for_pos(pos_t const &) const;
    void detect_screen_layout();
    void read_screen_layout_from_file(std::string const &);
    void add_monitor(int mon, int x, int y, int w, int h, Window);
    void subscribe_to_motion_events(Window);

    pos_t last_pos;
    file_descriptor xfd;
    logger & log;
    context & ctx;
    int xi_opcode = 0;
    dsp_t dsp;
    std::vector<rect> border_rects;
    std::vector<PointerBarrier> barriers;
    std::vector<monitor_t> monitors;
    bool hidden = false;
    int width = 0;
    int height = 0;
};
