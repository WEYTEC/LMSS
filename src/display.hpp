#pragma once

#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include <memory>
#include <vector>

#include "context.hpp"
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
        int x;
        int y;
        int w;
        int h;
    };

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
