/* SPDX-License-Identifier: BSD-3-Clause */

#include "display.hpp"

#include <sstream>

#include "types.hpp"

static const int BORDER_WIDTH = 1;
static const int BORDER_CLEARANCE = 8;
static const int RESOLUTION = 65536;

display::display(logger & log, context & ctx)
    : log(log)
    , ctx(ctx) {

    auto dsp_var = getenv("DISPLAY");
    dsp = dsp_t(XOpenDisplay(dsp_var));

    if (!dsp) {
        throw std::runtime_error("Failed to open display");
    }

    int event, error;
    if (!XQueryExtension(dsp.get(), "XInputExtension", &xi_opcode, &event, &error)) {
        throw std::runtime_error("XInput extension not supported");
    }

    int major = 2;
    int minor = 0;
    int retval = XIQueryVersion(dsp.get(), &major, &minor);
    if (retval != Success) {
        throw std::runtime_error("XInput 2.0 not supported");
    }

    unsigned char mask_bytes[(XI_LASTEVENT + 7) / 8] = {0};
    XISetMask(mask_bytes, XI_Motion);

    XIEventMask evmasks[1];
    evmasks[0].deviceid = XIAllDevices;
    evmasks[0].mask_len = sizeof(mask_bytes);
    evmasks[0].mask = mask_bytes;

    int screens = XScreenCount(dsp.get());
    log.info("display has " + std::to_string(screens) + " screens");

    for (auto s = 0; s < screens; ++s) {
        auto screen = XScreenOfDisplay(dsp.get(), s);
        if (!screen) {
            throw std::runtime_error("failed to get screen for display");
        }

        auto root = XRootWindowOfScreen(screen);

        int monitor_count = 0;
        XRRMonitorInfo * moninfo = XRRGetMonitors(dsp.get(), root, 1, &monitor_count);
        for (auto i = 0; i < monitor_count; ++i) {
            auto & mi = moninfo[i];
            std::stringstream ss;
            ss << "monitor " << i << " of screen " << s << ": " << mi.x << " " << mi.y << " "
               << mi.width << "x" << mi.height;
            log.info(ss.str());

            border_rects.emplace_back(mi.x, mi.y, BORDER_WIDTH, mi.height, i, border_t::LEFT);
            border_rects.emplace_back(mi.x, mi.y, mi.width, BORDER_WIDTH, i, border_t::TOP);
            border_rects.emplace_back(mi.x + mi.width - BORDER_WIDTH, mi.y, BORDER_WIDTH, mi.height,
                                      i, border_t::RIGHT);
            border_rects.emplace_back(mi.x, mi.y + mi.height - BORDER_WIDTH, mi.width, BORDER_WIDTH,
                                      i, border_t::BOTTOM);

            monitors.emplace_back(monitor_t { .id = i, .x = mi.x, .y = mi.y, .w = mi.width, .h = mi.height });
            width += mi.width;
            height += mi.height;
        }

        log.info("display size: " + std::to_string(width) + "x" + std::to_string(height));

        if (auto mask = XISelectEvents(dsp.get(), root, evmasks, 1); mask > 0) {
            throw std::runtime_error("failed to select XI events");
        }
    }

    XSync(dsp.get(), False);
    xfd = file_descriptor(ConnectionNumber(dsp.get()));
    ctx.get_el().add_fd(*xfd, std::bind(&display::handle_events, this, std::placeholders::_1));
}

void display::set_mouse_pos(mouse_pos_t const & mp) {
    std::stringstream ss;
    ss << "setting mouse pos on screen " << std::to_string(mp.screen)
       << " border: " << std::to_string(mp.border)
       << " pos:" << mp.pos;
    log.debug(ss.str());

    int x, y;
    hidden = false;
    switch (mp.border) {
        case border_t::TOP:
            x = monitors[mp.screen].x + monitors[mp.screen].w * mp.pos / RESOLUTION;
            y = monitors[mp.screen].y + BORDER_CLEARANCE;
            break;
        case border_t::BOTTOM:
            x = monitors[mp.screen].x + monitors[mp.screen].w * mp.pos / RESOLUTION;
            y = monitors[mp.screen].y + monitors[mp.screen].h - BORDER_CLEARANCE;
            break;
        case border_t::LEFT:
            x = monitors[mp.screen].x + BORDER_CLEARANCE;
            y = monitors[mp.screen].y + monitors[mp.screen].h * mp.pos / RESOLUTION;
            break;
        case border_t::RIGHT:
            x = monitors[mp.screen].x + monitors[mp.screen].w - BORDER_CLEARANCE;
            y = monitors[mp.screen].y + monitors[mp.screen].h * mp.pos / RESOLUTION;
            break;
        case border_t::HIDE:
            log.debug("hiding cursor");
            // we position the pointer in the left bottom border, there is still 1 pixel visible
            // maybe we could really hide it by overriding the cursor bitmap
            x = monitors[mp.screen].x;
            y = monitors[mp.screen].y + height;
            hidden = true;
            break;
        default:
            log.debug("centering cursor");
            x = monitors[mp.screen].x + monitors[mp.screen].w / 2;
            y = monitors[mp.screen].y + monitors[mp.screen].h / 2;
    }
    XWarpPointer(dsp.get(), None, XDefaultRootWindow(dsp.get()), 0, 0, 0, 0, x, y);
    last_pos = { x, y };
    XFlush(dsp.get());
}

void display::handle_events(int) {
    XEvent ev;
    while (XPending(dsp.get())) {
        XNextEvent(dsp.get(), &ev);

        if (ev.xcookie.type == GenericEvent && ev.xcookie.extension == xi_opcode) {
            if (!XGetEventData(dsp.get(), &ev.xcookie)) {
                log.warn("failed to get X event data");
                continue;
            }

            if (ev.xcookie.evtype != XI_Motion || hidden) {
                XFreeEventData(dsp.get(), &ev.xcookie);
                continue;
            }
            XFreeEventData(dsp.get(), &ev.xcookie);

            Window root, child;
            int root_x, root_y;
            int x, y;
            unsigned int mask;

            if (!XQueryPointer(dsp.get(), XDefaultRootWindow(dsp.get()), &root, &child, &root_x, &root_y,
                &x, &y, &mask)) {

                log.warn("X event for other screen?");
                continue;
            }
            log.debug("pointer: " + std::to_string(root_x) + "/" + std::to_string(root_y));

            if (mask & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) {
                log.debug("mouse button pressed, skipping border detection");
                continue;
            }

            // we need to check if we crossed a border since last pointer update
            if (std::abs(root_x - last_pos.x) > 1 || std::abs(root_y - last_pos.y) > 1) {
                auto const & m_last = get_mon_for_pos(last_pos);
                auto const & m_cur = get_mon_for_pos({root_x, root_y});

                if (m_last != m_cur) {
                    border_t border;
                    uint16_t pos = 0;
                    if (m_cur.x == m_last.x) {
                        log.debug("mons are above each other: "
                                  + std::to_string(m_cur.y) + "+" + std::to_string(m_cur.h) + " | "
                                  + std::to_string(m_last.y) + "+" + std::to_string(m_last.h));

                        pos = RESOLUTION * (root_x - m_cur.x) / m_cur.w;
                        border = m_cur.y + m_cur.h == m_last.y ? border_t::TOP : border_t::BOTTOM;
                    } else {
                        log.debug("mons are next to each other: "
                                  + std::to_string(m_cur.x) + "+" + std::to_string(m_cur.w) + " | "
                                  + std::to_string(m_last.x) + "+" + std::to_string(m_last.w));

                        border = m_cur.x + m_cur.w == m_last.x ? border_t::LEFT : border_t::RIGHT;
                        pos = RESOLUTION * (root_y - m_cur.y) / m_cur.h;
                    }
                    log.debug("border: " + std::to_string(border));

                    ctx.mouse_at_border({
                        .screen = static_cast<uint8_t>(m_last.id),
                        .border = border,
                        .pos = pos
                    });
                }
            } else {
                for (auto & b : border_rects) {
                    if (b.inside(root_x, root_y)) {
                        uint16_t pos = 0;
                        switch (b.border) {
                            case border_t::TOP:
                            case border_t::BOTTOM:
                                pos = RESOLUTION * (root_x - b.x) / b.w;
                                break;
                            case border_t::LEFT:
                            case border_t::RIGHT:
                                pos = RESOLUTION * (root_y - b.y) / b.h;
                                break;
                            default:
                                throw std::runtime_error("invalid border");
                        }

                        log.debug("pointer at border " + std::to_string(b.border) + " of screen "
                            + std::to_string(b.screen));

                        ctx.mouse_at_border({
                            .screen = static_cast<uint8_t>(b.screen),
                            .border = b.border,
                            .pos = pos
                        });

                        break;
                    }
                }
            }
            last_pos = { root_x, root_y };
        }
    }
}

display::monitor_t const & display::get_mon_for_pos(pos_t const & pos) const {
    for (auto const & m : monitors) {
        if (pos.x >= m.x && pos.x <= m.x + m.w && pos.y >= m.y && pos.y <= m.y + m.h) {
            return m;
        }
    }

    throw std::logic_error("pointer not inside of any known monitor");
}
