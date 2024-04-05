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
    display(logger &, bool dnd, context &);
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

    struct barrier_t {
        PointerBarrier id = 0;
        border_t border;
        int mon = 0;
    };

    void detect_screen_layout();
    void read_screen_layout_from_file(std::string const &);
    void add_monitor(int mon, int x, int y, int w, int h, Window);
    void subscribe_to_barrier_events(Window);

    size_t last_mon = 0;
    file_descriptor xfd;
    logger & log;
    bool enable_dnd;
    context & ctx;
    int xi_opcode = 0;
    dsp_t dsp;
    std::vector<barrier_t> barriers;
    std::vector<monitor_t> monitors;
    bool hidden = false;
};
