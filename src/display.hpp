#pragma once

#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include <memory>
#include <vector>

#include "context.hpp"
#include "file_descriptor.hpp"
#include "logger.hpp"
#include "rect.hpp"

class display final {
public:
    display(logger &, context &);

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

        inline bool operator==(monitor_t const & other) const {
            return std::tie(id, x, y, w, h) ==
                std::tie(other.id, other.x, other.y, other.w, other.h);
        }

        inline bool operator!=(monitor_t const & other) const { return !operator==(other); }
    };

    struct pos_t {
        int x = 0;
        int y = 0;
    };

    monitor_t const & get_mon_for_pos(pos_t const &) const;

    pos_t last_pos;
    file_descriptor xfd;
    logger & log;
    context & ctx;
    int xi_opcode = 0;
    dsp_t dsp;
    std::vector<rect> border_rects;
    std::vector<monitor_t> monitors;
    bool hidden = false;
    int width = 0;
    int height = 0;
};
