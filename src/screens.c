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
#include "screens.h"
#include "utilities.h"

//--------------------------------------------------------
// gen_smart_touch_screens
//
// Routine to sort the screens by position on a desktop canvas for the Smart Touch
// because the IP switching protocol does not deliver screen information unlike USB switching
// recreate compatible data structures
//--------------------------------------------------------
void gen_smart_touch_screens(void) {

    struct st_monitors {
        int xstart;
        int ystart;
        int width;
        int height;
    } st_mons[MONITOR_LIMIT];

    int scr = DefaultScreen(ctx.display);
    Window root = RootWindow(ctx.display, scr);
    int monitor_number = 0;
    XRRMonitorInfo * moninfo = XRRGetMonitors(ctx.display, root, 1, &monitor_number);
    ctx.monitors_count = moninfo ? (monitor_number > MONITOR_LIMIT ? MONITOR_LIMIT : monitor_number) : 0;
    ctx.monitors = moninfo;
    slog(LL_INFO, "Number of monitors = %d", ctx.monitors_count);
    ctx.ws = 200;  // Dummy WS for this PC

    int xmin = 0;
    int ymin = 0;
    int xmax = 0;
    int ymax = 0;
    int totalstepsx = 0;
    int totalstepsy = 0;

    for (int i = 0; i < ctx.monitors_count; i++) {
        st_mons[i].xstart = ctx.monitors[i].x;
        st_mons[i].ystart = ctx.monitors[i].y;
        st_mons[i].width = ctx.monitors[i].width;
        st_mons[i].height = ctx.monitors[i].height;

        if (st_mons[i].xstart <= xmin) xmin = st_mons[i].xstart;
        if (st_mons[i].ystart <= ymin) ymin = st_mons[i].ystart;
        if (st_mons[i].xstart + st_mons[i].width >= xmax) xmax = st_mons[i].xstart + st_mons[i].width;
        if (st_mons[i].ystart + st_mons[i].height >= ymax) ymax = st_mons[i].ystart + st_mons[i].height;

        slog(LL_INFO, "i is %d Screen x %4d y %4d w %4d h %4d", i, st_mons[i].xstart, st_mons[i].ystart,
            st_mons[i].width, st_mons[i].height);
    }

    int stepx[MONITOR_LIMIT] = {0};
    size_t const stepsizex = sizeof stepx / sizeof stepx[0];
    int stepy[MONITOR_LIMIT] = {0};
    size_t const stepsizey = sizeof stepy / sizeof stepy[0];

    //------Get all intersections
    {
        int j = 0;
        int k = 0;
        for (int i = 0; i < ctx.monitors_count; i++) {
            stepx[j++] = st_mons[i].xstart;
            stepy[k++] = st_mons[i].ystart;
            stepx[j++] = st_mons[i].width;
            stepy[k++] = st_mons[i].height;
        }
        stepx[j] = xmax;
        stepy[k] = ymax;

        totalstepsx = j;
        totalstepsy = k;
    }

    if (is_log_level(LL_DEBUG)) {
        for (int i = 0; i <= totalstepsx; i++) {
            slog(LL_DEBUG, "stepx[%d] %d", i, stepx[i]);
        }

        for (int i = 0; i <= totalstepsy; i++) {
            slog(LL_DEBUG, "stepy[%d] %d", i, stepy[i]);
        }
    }

    merge_sort(stepx, stepx + stepsizex);
    compact(stepx, stepx + stepsizex);

    merge_sort(stepy, stepy + stepsizey);
    compact(stepy, stepy + stepsizey);

    if (is_log_level(LL_DEBUG)) {
        for (int i = 0; i <= totalstepsx; i++) {
            slog(LL_DEBUG, "**after merge sort stepx[%d] %d", i, stepx[i]);
        }

        for (int i = 0; i <= totalstepsy; i++) {
            slog(LL_DEBUG, "**after merge sort stepy[%d] %d", i, stepy[i]);
        }

        for (int i = 0; i <= totalstepsx; i++) {
            slog(LL_DEBUG, "stepx[%d] %d", i, stepx[i]);
        }

        for (int i = 0; i <= totalstepsy; i++) {
            slog(LL_DEBUG, "stepy[%d] %d", i, stepy[i]);
        }

        slog(LL_DEBUG, " xmin %d, ymin %d, xmax %d and ymax %d", xmin, ymin, xmax, ymax);
    }

    //----fill in virtual screens and use index to save monitor id - set all l, r, u and d to 255
    for (int r = 0; r < MONITOR_LIMIT; r++) {
        for (int c = 0; c < MONITOR_LIMIT; c++) {
            for (int i = 0; i < ctx.monitors_count; i++) {
                if (c < totalstepsx && r < totalstepsy && stepx[c] >= st_mons[i].xstart &&
                    stepx[c] < (st_mons[i].xstart + st_mons[i].width) && stepy[r] >= st_mons[i].ystart &&
                    stepy[r] < (st_mons[i].ystart + st_mons[i].height)) {

                    slog(LL_DEBUG, "hit r %d c%d", r, c);
                    slog(LL_DEBUG, "stepx[c]  %d st_mons[i].xstart %d", stepx[c], st_mons[i].xstart);
                    slog(LL_DEBUG, "stepx[c+1] %d st_mons[i].xstart+st_mons[i].width %d", stepx[c + 1],
                        st_mons[i].xstart + st_mons[i].width);
                    slog(LL_DEBUG, "stepy[r] %d st_mons[i].ystart %d", stepy[r], st_mons[i].ystart);
                    slog(LL_DEBUG, "stepy[r+1] %d st_mons[i].ystart+st_mons[i].height %d", stepy[r + 1],
                        st_mons[i].ystart + st_mons[i].height);

                    ctx.my_mons[r][c].index = i;
                    ctx.my_mons[r][c].monx = stepx[c + 1] - stepx[c];
                    ctx.my_mons[r][c].mony = stepy[r + 1] - stepy[r];
                    ctx.my_mons[r][c].monl = 255;
                    ctx.my_mons[r][c].monr = 255;
                    ctx.my_mons[r][c].monu = 255;
                    ctx.my_mons[r][c].mond = 255;
                }
            }
        }
    }

    //---- Check for monitors l, r, u and d and if present set WS to 200
    for (int r = 0; r < MONITOR_LIMIT; r++) {
        for (int c = 0; c < MONITOR_LIMIT; c++) {
            if (c > 0 && ctx.my_mons[r][c].monr > 0 && ctx.my_mons[r][c - 1].monl > 0) {
                ctx.my_mons[r][c].monl = ctx.ws;
            }

            if (c < (MONITOR_LIMIT - 1) && ctx.my_mons[r][c].monr > 0 && ctx.my_mons[r][c + 1].monr > 0) {
                ctx.my_mons[r][c].monr = ctx.ws;
            }

            if (r > 0 && ctx.my_mons[r][c].monu > 0 && ctx.my_mons[r - 1][c].monu > 0) {
                ctx.my_mons[r][c].monu = ctx.ws;
            }

            if (r < (MONITOR_LIMIT - 1) && ctx.my_mons[r][c].mond > 0 && ctx.my_mons[r + 1][c].mond > 0) {
                ctx.my_mons[r][c].mond = ctx.ws;
            }
        }
    }

    //----Complete Screen arrays
    for (int r = 0; r < totalstepsy; r++) {
        for (int c = 0; c < totalstepsx; c++) {
            ctx.screen_w[c].start = stepx[c];
            ctx.screen_w[c].width = stepx[c + 1];
            ctx.screen_w[c].col = c;

            ctx.screen_h[r].start = stepy[r];
            ctx.screen_h[r].height = stepy[r + 1];
            ctx.screen_h[r].row = r;
        }
    }

    ctx.total_h = ymax;
    ctx.total_w = xmax;
}
