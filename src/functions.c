/*
Mouse Switching Software
for Linux and Unix based operating systems

BSD 3-Clause License

Copyright (c) 2020, WEY Technology AG
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "functions.h"
#include "log.h"
#include "weyip_usb.h"
#include <stdint.h>
#include <sys/time.h>

#define WEYIP_RESOLUTION 65536

struct context ctx = {0};  // initialize all with zeros

void update_ctx_now(void) {
    struct timeval now;

    gettimeofday(&now, NULL);
    ctx.now_sec = now.tv_sec;
    ctx.now_usec = now.tv_usec;

    slog(LL_DEBUG, "now = %ld.%06ld was = %ld.%06ld", ctx.now_sec, ctx.now_usec, ctx.was_sec, ctx.was_usec);
}

void check_usb(void) {
    int weyip_enum_err = enumerate_weyip_usb();

    slog(LL_DEBUG, "enumerate_weyip_usb() returned %d\r", weyip_enum_err);

    if (!weyip_enum_err) {
        ctx.configured = true;
    }

    if (!weyip_enum_err && is_log_level(LL_DEBUG)) {
        for (int j = 0; j < 16; j++) {
            slog(LL_DEBUG, "Start %4d height %4d and row %2d", ctx.screen_h[j].start, ctx.screen_h[j].height + 1,
                ctx.screen_h[j].row);
        }

        for (int i = 0; i < 16; i++) {
            slog(LL_DEBUG, "Start %4d width  %4d and col %2d", ctx.screen_w[i].start, ctx.screen_w[i].width + 1,
                ctx.screen_w[i].col);
        }

        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                slog(LL_DEBUG, "index %4d, x %4d, y %4d, l %3d, r %3d, u %3d, d %3d", ctx.my_mons[i][j].index,
                    ctx.my_mons[i][j].monx, ctx.my_mons[i][j].mony, ctx.my_mons[i][j].monl, ctx.my_mons[i][j].monr,
                    ctx.my_mons[i][j].monu, ctx.my_mons[i][j].mond);
            }
        }
    }
}

void heartbeat(void) {
    if (ctx.now_sec != ctx.was_sec) {
        slog(LL_INFO, "Tick = %ld", ctx.now_sec);

        if (ctx.configured) {
            slog(LL_DEBUG, "Sending WEYIP timer message ");

            // sending mouse positioning done, workaround for Solaris
            send_mouse_positioning_done();

            // sending heartbeat
            int err = send_heartbeat();
            slog(LL_DEBUG, " done send_heartbeat %d", err);
        }
    }
}

void mouse_at_border_eval(void) {
    // get info about current pointer position
    bool xpointer_ret = XQueryPointer(ctx.display, RootWindow(ctx.display, DefaultScreen(ctx.display)), &ctx.xbe.root,
        &ctx.xbe.window, &ctx.xbe.x_root, &ctx.xbe.y_root, &ctx.xbe.x, &ctx.xbe.y, &ctx.xbe.state);

    int MouseButtonState = ctx.xbe.state & MOUSE_BUTTON_MASK;

    if (is_log_level(LL_DEBUG)) {
        slog(LL_DEBUG, "Mouse: X=%04d Y=%04d (%dx%d) MouseButtonState %04x returned %s", ctx.xbe.x, ctx.xbe.y,
            WidthOfScreen(DefaultScreenOfDisplay(ctx.display)), HeightOfScreen(DefaultScreenOfDisplay(ctx.display)),
            ctx.xbe.state, xpointer_ret ? "true" : "false");
        slog(LL_DEBUG, "MOUSE_BUTTON_MASK = %04x and MouseButtonState %04x", MOUSE_BUTTON_MASK, MouseButtonState);

        slog(LL_DEBUG, "Mouse: X=%04d Y=%04d	Root X=%04d Y=%04d", ctx.xbe.x, ctx.xbe.y, ctx.xbe.x_root,
            ctx.xbe.y_root);
    }

    int data = 0;
    int Monitor = 0;
    int maxw = 0;
    int maxh = 0;
    int hstart = 0;
    int wstart = 0;

    int row = 0;
    int col = 0;

    int position = 0;

    // which screen?
    if (ctx.configured && !MouseButtonState) {
        slog(LL_DEBUG, "Which screen are we on?");

        for (int j = 0; ctx.screen_h[j].height > 0 && j < 16; j++) {
            if (ctx.xbe.y < ctx.screen_h[j].height) {
                slog(LL_DEBUG, "Which screen start %4d height %4d and row %2d", ctx.screen_h[j].start,
                    ctx.screen_h[j].height + 1, ctx.screen_h[j].row);

                row = ctx.screen_h[j].row;
                maxh = ctx.screen_h[j].height;
                hstart = ctx.screen_h[j].start;

                for (int i = 0; ctx.screen_w[i].width > 0 && i < 16; i++) {
                    if (ctx.xbe.x < ctx.screen_w[i].width) {

                        slog(LL_DEBUG, "Screen start %4d width  %4d and col %2d", ctx.screen_w[i].start,
                            ctx.screen_w[i].width + 1, ctx.screen_w[i].col);

                        col = ctx.screen_w[i].col;
                        maxw = ctx.screen_w[i].width;
                        wstart = ctx.screen_w[i].start;
                        break;
                    }
                }
                break;
            }
        }

        if (is_log_level(LL_DEBUG)) {
            slog(LL_DEBUG, "Screen row %d col %d hstart %4d maxh %4d wstart %4d and maxw %4d", row, col, hstart, maxh,
                wstart, maxw);
            slog(LL_DEBUG, "---------------------------------------------------------------------");
            slog(LL_DEBUG, "Row %2d, Col %2d, Id %3d ws %d", row, col, ctx.my_mons[row][col].index, ctx.ws);
            slog(LL_DEBUG, "posx %4d, posy %4d, x %4d, y %4d", ctx.xbe.x, ctx.xbe.y, ctx.my_mons[row][col].monx,
                ctx.my_mons[row][col].mony);
            slog(LL_DEBUG, "l %3d, r %3d, u %3d, d %3d, wstart, %4d, hstart %4d, maxw %4d, maxh %4d",
                ctx.my_mons[row][col].monl, ctx.my_mons[row][col].monr, ctx.my_mons[row][col].monu,
                ctx.my_mons[row][col].mond, wstart, hstart, maxw, maxh);
            slog(LL_DEBUG, "---------------------------------------------------------------------");
        }

        // top outer
        if (ctx.xbe.y == hstart && ctx.my_mons[row][col].monu != ctx.ws && ctx.my_mons[row][col].monu > 0) {

            slog(LL_INFO, "TOP o   mouse = %4d:%4d u %d, Mon %2x, hstart, %4d data_send %hhd", ctx.xbe.x, ctx.xbe.y,
                ctx.my_mons[row][col].monu, ctx.my_mons[row][col].index, hstart, ctx.data_send);

            data = 1;
            Monitor = ctx.my_mons[row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.x - wstart) / (maxw - wstart + 1);
        }
        // top inner
        else if (!ctx.do_not_send && ctx.last_row != row && ctx.last_row > row &&
            ctx.my_mons[row][col].index != ctx.my_mons[ctx.last_row][col].index) {

            slog(LL_INFO, "TOP i   mouse = %4d:%4d u %d, Mon %2x, hstart, %4d data_send %hhd", ctx.xbe.x, ctx.xbe.y,
                ctx.my_mons[ctx.last_row][col].monu, ctx.my_mons[ctx.last_row][col].index, hstart, ctx.data_send);

            data = 1;
            Monitor = ctx.my_mons[ctx.last_row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.x - wstart) / (maxw - wstart + 1);
        }
        // bottom outer
        else if (!(ctx.xbe.y == ctx.total_h - 1 && ctx.xbe.x == 1) &&
            (ctx.xbe.y == hstart + ctx.my_mons[row][col].mony - 1) && (ctx.my_mons[row][col].mond != ctx.ws) &&
            (ctx.my_mons[row][col].mond > 0) && ctx.xbe.x != 0) {

            slog(LL_INFO, "BOTTOM o mouse = %4d:%4d d %d, Mon %2x, hstart, %4d Height %4d data_send %hhd", ctx.xbe.x,
                ctx.xbe.y, ctx.my_mons[row][col].mond, ctx.my_mons[row][col].index, hstart, ctx.my_mons[row][col].mony,
                ctx.data_send);

            data = 2;
            Monitor = ctx.my_mons[row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.x - wstart) / (maxw - wstart + 1);
        }
        // bottom inner
        else if (!ctx.do_not_send && ctx.last_row != row && ctx.last_row < row &&
            ctx.my_mons[row][col].index != ctx.my_mons[ctx.last_row][col].index) {

            slog(LL_INFO, "BOTTOM i mouse = %4d:%4d d %d, Mon %2x, hstart, %4d Height %4d data_send %hhd", ctx.xbe.x,
                ctx.xbe.y, ctx.my_mons[ctx.last_row][col].mond, ctx.my_mons[ctx.last_row][col].index, hstart,
                ctx.my_mons[row][col].mony, ctx.data_send);

            data = 2;
            Monitor = ctx.my_mons[ctx.last_row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.x - wstart) / (maxw - wstart + 1);
        }
        // left outer
        else if (ctx.xbe.x == wstart && ctx.my_mons[row][col].monl != ctx.ws && ctx.my_mons[row][col].monl > 0 &&
            ctx.xbe.y != ctx.total_h - 1) {

            slog(LL_INFO, "LEFT o  mouse = %4d:%4d l %d, Mon %2x, wstart, %4d data_send %hhd", ctx.xbe.x, ctx.xbe.y,
                ctx.my_mons[row][col].monl, ctx.my_mons[row][col].index, wstart, ctx.data_send);

            data = 3;
            Monitor = ctx.my_mons[row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.y - hstart) / (maxh - hstart + 1);
        }
        // left inner
        else if (!ctx.do_not_send && ctx.last_col != col && ctx.last_col > col &&
            ctx.my_mons[row][ctx.last_col].index != ctx.my_mons[row][col].index) {

            slog(LL_INFO, "LEFT i  mouse = %4d:%4d l %d, Mon %2x, wstart, %4d data_send %hhd", ctx.xbe.x, ctx.xbe.y,
                ctx.my_mons[row][ctx.last_col].monl, ctx.my_mons[row][ctx.last_col].index, wstart, ctx.data_send);

            data = 3;
            Monitor = ctx.my_mons[row][ctx.last_col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.y - hstart) / (maxh - hstart + 1);
        }
        // right outer
        else if (ctx.xbe.x == wstart + ctx.my_mons[row][col].monx - 1 && ctx.my_mons[row][col].monr != ctx.ws &&
            ctx.my_mons[row][col].monr > 0) {

            slog(LL_INFO,
                "RIGHT o mouse = %4d:%4d r %d, Mon %2x, wstart %4d, hstart %4d, maxh %d, Width %4d data_send %hhd",
                ctx.xbe.x, ctx.xbe.y, ctx.my_mons[row][col].monr, ctx.my_mons[row][col].index, wstart, hstart, maxh,
                ctx.my_mons[row][col].monx, ctx.data_send);

            data = 4;
            Monitor = ctx.my_mons[row][col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.y - hstart) / (maxh - hstart + 1);
        }
        // right inner
        else if (!ctx.do_not_send && ctx.last_col != col && ctx.last_col < col &&
            ctx.my_mons[row][ctx.last_col].index != ctx.my_mons[row][col].index) {

            slog(LL_INFO,
                "RIGHT i mouse = %4d:%4d r %d, Mon %2x, wstart %4d, hstart %4d, maxh %d, Width %4d data_send %hhd",
                ctx.xbe.x, ctx.xbe.y, ctx.my_mons[row][ctx.last_col].monr, ctx.my_mons[row][ctx.last_col].index, wstart,
                hstart, maxh, ctx.my_mons[row][ctx.last_col].monx, ctx.data_send);

            data = 4;
            Monitor = ctx.my_mons[row][ctx.last_col].index;
            position = WEYIP_RESOLUTION * (ctx.xbe.y - hstart) / (maxh - hstart + 1);
        } else {
            ctx.data_send = 0;
        }

        slog(LL_DEBUG, "Data is %d and data_send is %hhd", data, ctx.data_send);

        if (data) {
            slog(LL_DEBUG, "Row %2d, Col %2d, Id %3d, x %4d, y %4d, l %3d, r %3d, u %3d, d %3d, maxw %4d, maxh %4d",
                row, col, ctx.my_mons[row][col].index, ctx.my_mons[row][col].monx, ctx.my_mons[row][col].mony,
                ctx.my_mons[row][col].monl, ctx.my_mons[row][col].monr, ctx.my_mons[row][col].monu,
                ctx.my_mons[row][col].mond, maxw, maxh);
            slog(LL_DEBUG, "Virtual desktop is %4d x %4d", ctx.total_w, ctx.total_h);
            slog(LL_DEBUG, " Border %d Monitor %d position %d *****************", data, Monitor, position);
        }

        ctx.last_row = row;
        ctx.last_col = col;
        ctx.do_not_send = false;

        // send data to HID device
        if (data > 0 && !ctx.data_send)  // send data only once
        {
            int err = send_mouse_position(Monitor, data - 1, position);
            slog(LL_DEBUG, "----- send_mouse_position %d", err);
            slog(LL_DEBUG, "    data is %d and data_send is %hhd", data, ctx.data_send);
            if (!err) {
                ctx.data_send = 1;
                ctx.data_recv = 0;
            }

            slog(LL_DEBUG, "Now data is %d and data_send is %hhd", data, ctx.data_send);
        }
    }
}

void set_mouse_on_message(void) {
    if (!ctx.data_recv) {
        slog(LL_DEBUG, "RX01 ");

        uint8_t screen;
        uint8_t border;
        uint16_t position;

        int posx = 0;
        int posy = 0;

        if (!read_mouse_position(&screen, &border, &position) && screen < ctx.monitors_count) {
            slog(LL_DEBUG, "Going into Switch where border is %hhu, and screen %hhu", border, screen);

            switch (border) {
                case 0:
                    slog(LL_INFO, "Top:    position is %u", (unsigned)position);
                    posx = ctx.monitors[screen].x + position * ctx.monitors[screen].width / WEYIP_RESOLUTION;
                    posy = ctx.monitors[screen].y;
                    slog(LL_DEBUG, "screen is %hhu, posx is %d, posy %d, width %d and height %d", screen, posx, posy,
                        ctx.monitors[screen].width, ctx.monitors[screen].height);
                    break;
                case 1:
                    slog(LL_INFO, "Bottom: position is %u", (unsigned)position);
                    posx = ctx.monitors[screen].x + position * ctx.monitors[screen].width / WEYIP_RESOLUTION;
                    posy = ctx.monitors[screen].y + ctx.monitors[screen].height - 2;
                    slog(LL_DEBUG, "screen is %hhu, posx is %d, posy %d, width %d and height %d", screen, posx, posy,
                        ctx.monitors[screen].width, ctx.monitors[screen].height);
                    break;
                case 2:
                    slog(LL_INFO, "Left:   position is %u", (unsigned)position);
                    posx = ctx.monitors[screen].x;
                    posy = ctx.monitors[screen].y + position * ctx.monitors[screen].height / WEYIP_RESOLUTION;
                    slog(LL_DEBUG, "screen is %hhu, posx is %d, posy %d, width %d and height %d", screen, posx, posy,
                        ctx.monitors[screen].width, ctx.monitors[screen].height);
                    break;
                case 3:
                    slog(LL_INFO, "Right:  position is %u", (unsigned)position);
                    posx = ctx.monitors[screen].x + ctx.monitors[screen].width - 2;
                    posy = ctx.monitors[screen].y + position * ctx.monitors[screen].height / WEYIP_RESOLUTION;
                    slog(LL_DEBUG, "screen is %hhu, posx is %d, posy %d, width %d and height %d", screen, posx, posy,
                        ctx.monitors[screen].width, ctx.monitors[screen].height);
                    break;
                case 4:
                    slog(LL_DEBUG, "Hide:");
                    posy = ctx.total_h - 1 + 300;
                    posx = 1 - 300;
                    break;
                default:
                    slog(LL_DEBUG, "Centre:");
                    posy = ctx.total_h / 2;
                    posx = ctx.total_w / 2;
                    break;
            }

            ctx.do_not_send = true;

            //--------------------------------------------------------
            // Set mouse pointer position
            //--------------------------------------------------------
            slog(LL_DEBUG, "Place at: X=%04d Y=%04d Mouse: X=%04d Y=%04d  ", posx, posy, posx - ctx.xbe.x,
                posy - ctx.xbe.y);

            XWarpPointer(ctx.display, None, None, 0, 0, 0, 0, posx - ctx.xbe.x, posy - ctx.xbe.y);

            XQueryPointer(ctx.display, RootWindow(ctx.display, DefaultScreen(ctx.display)), &ctx.xbe.root,
                &ctx.xbe.window, &ctx.xbe.x_root, &ctx.xbe.y_root, &ctx.xbe.x, &ctx.xbe.y, &ctx.xbe.state);

            slog(LL_DEBUG, "Now: X=%04d Y=%04d", ctx.xbe.x, ctx.xbe.y);
            slog(LL_DEBUG, "RX02 ");

            //--------------------------------------------------------
            // Send mouse position done to HID device
            //--------------------------------------------------------
            int err = send_mouse_positioning_done();
            slog(LL_DEBUG, "----- send_mouse_positioning_done %d", err);

            ctx.data_send = !err ? 1 : 0;
            ctx.data_recv = 1;
        }
    }
}
