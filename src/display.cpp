/* SPDX-License-Identifier: BSD-3-Clause */

#include "display.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include "types.hpp"

static const char screen_config_file[] = "/etc/lmss.sl";

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

    if (std::filesystem::exists(screen_config_file)) {
        read_screen_layout_from_file(screen_config_file);
    } else {
        detect_screen_layout();
    }

    XSync(dsp.get(), False);
    xfd = file_descriptor(ConnectionNumber(dsp.get()));
    ctx.get_el().add_fd(*xfd, std::bind(&display::handle_events, this, std::placeholders::_1));
}

void display::read_screen_layout_from_file(std::string const & config_file) {
    std::ifstream cf(config_file);

    std::regex screen_regex("^([0-9]+)x([0-9]+)\\+([0-9]+)\\+([0-9]+$)");
    std::smatch screen_match;

    int mon = 0;
    for (std::string line; std::getline(cf, line);) {
        if (std::regex_search(line, screen_match, screen_regex)) {
            log.debug("adding screen with config: " + line);
            auto w = std::stoi(screen_match[1]);
            auto h = std::stoi(screen_match[2]);
            auto x = std::stoi(screen_match[3]);
            auto y = std::stoi(screen_match[4]);
            auto root = XDefaultRootWindow(dsp.get());

            add_monitor(mon, x, y, w, h, root);

        } else {
            throw std::runtime_error("failed to parse screen configuration: " + line);
        }

        mon++;
    }

    if (mon == 0) {
        throw std::runtime_error("empty screen layout configuration");
    }

    subscribe_to_motion_events(XDefaultRootWindow(dsp.get()));
}

void display::subscribe_to_motion_events(Window win) {
    unsigned char mask_bytes[(XI_LASTEVENT + 7) / 8] = {0};
    XISetMask(mask_bytes, XI_RawMotion);

    XIEventMask evmasks[1];
    evmasks[0].deviceid = XIAllDevices;
    evmasks[0].mask_len = sizeof(mask_bytes);
    evmasks[0].mask = mask_bytes;

    if (auto mask = XISelectEvents(dsp.get(), win, evmasks, 1); mask > 0) {
        throw std::runtime_error("failed to select XI events");
    }
}

void display::detect_screen_layout() {
    int screens = XScreenCount(dsp.get());
    log.info("display has " + std::to_string(screens) + " screens");

    size_t id = 0;
    for (auto s = 0; s < screens; ++s) {
        auto screen = XScreenOfDisplay(dsp.get(), s);
        if (!screen) {
            throw std::runtime_error("failed to get screen for display");
        }

        auto root = XRootWindowOfScreen(screen);

        int monitors = 0;
        XRRMonitorInfo * moninfo = XRRGetMonitors(dsp.get(), root, 1, &monitors);
        for (auto m = 0; m < monitors; ++m) {
            auto & mi = moninfo[m];
            std::stringstream ss;
            ss << "monitor " << m << " of screen " << s << ": " << mi.x << " " << mi.y << " "
               << mi.width << "x" << mi.height;
            log.info(ss.str());

            add_monitor(id, mi.x, mi.y, mi.width, mi.height, root);
            ++id;
        }

        log.info("display size: " + std::to_string(width) + "x" + std::to_string(height));
        subscribe_to_motion_events(root);
    }
}

void display::add_monitor(int mon, int x, int y, int w, int h, Window root) {
    border_rects.emplace_back(x, y, BORDER_WIDTH, h, mon, border_t::LEFT, root);
    border_rects.emplace_back(x, y, w, BORDER_WIDTH, mon, border_t::TOP, root);
    border_rects.emplace_back(x + w - BORDER_WIDTH, y, BORDER_WIDTH, h, mon, border_t::RIGHT, root);
    border_rects.emplace_back(x, y + h - BORDER_WIDTH, w, BORDER_WIDTH, mon, border_t::BOTTOM, root);

    monitors.emplace_back(monitor_t { .id = mon, .x = x, .y = y, .w = w, .h = h, .root = root });
    width = std::max(x + w, width);
    height = std::max(y + h, height);
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
    XWarpPointer(dsp.get(), None, monitors[mp.screen].root, 0, 0, 0, 0, x, y);
    last_pos = { x, y, monitors[mp.screen].root };
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

            if (ev.xcookie.evtype != XI_RawMotion || hidden) {
                XFreeEventData(dsp.get(), &ev.xcookie);
                continue;
            }
            XFreeEventData(dsp.get(), &ev.xcookie);

            Window root, child;
            int root_x, root_y;
            int x, y;
            unsigned int mask;

            for (auto & m : monitors) {
                if (XQueryPointer(dsp.get(), m.root, &root, &child, &root_x, &root_y,
                    &x, &y, &mask)) {

                    break;
                }
            }

            log.debug("pointer: " + std::to_string(root_x) + "/" + std::to_string(root_y));

            if (mask & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)) {
                log.debug("mouse button pressed, skipping border detection");
                continue;
            }

            auto const & m_last = get_mon_for_pos(last_pos);
            auto const & m_cur = get_mon_for_pos({root_x, root_y, root});
            auto diff_x = std::abs(root_x - last_pos.x);
            auto diff_y = std::abs(root_y - last_pos.y);
            uint16_t pos = 0;
            border_t border;

            // we need to check if we crossed a border since last pointer update
            if (root != last_pos.root) {
                if (diff_x < diff_y) { // top/bottom
                    pos = RESOLUTION * (last_pos.x - m_last.x) / m_last.w;
                    if (root_y < m_cur.h / 2) { // top
                        border = border_t::TOP;
                    } else { //bottom
                        border =  border_t::BOTTOM;
                    }
                } else { // left/right
                    pos = RESOLUTION * (last_pos.y - m_last.y) / m_last.h;
                    if (root_x < m_cur.w / 2) { // right
                        border = border_t::RIGHT;
                    } else { // left
                        border = border_t::LEFT;
                    }
                }

                log.debug("different root window, left at border: " + std::to_string(border));

                ctx.mouse_at_border({
                    .screen = static_cast<uint8_t>(m_last.id),
                    .border = border,
                    .pos = pos
                });
            } else if (m_last != m_cur && (diff_x >= 1 || diff_y >= 1)) {
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
            } else {
                for (auto & b : border_rects) {
                    if (b.inside(root_x, root_y) && b.root == root) {
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
            last_pos = { root_x, root_y, root };
        }
    }
}

display::monitor_t const & display::get_mon_for_pos(pos_t const & pos) const {
    for (auto const & m : monitors) {
        if ((pos.root == m.root || pos.root == 0)
            && pos.x >= m.x
            && pos.x <= m.x + m.w
            && pos.y >= m.y
            && pos.y <= m.y + m.h) {

            return m;
        }
    }

    throw std::logic_error("pointer not inside of any known monitor");
}
