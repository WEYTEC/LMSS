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

#pragma once

#include <stdbool.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

#define MONITOR_LIMIT 16
#define MOUSE_BUTTON_MASK (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)

struct my_monitors {
    int index;
    int monx;
    int mony;
    int monr;
    int monl;
    int monu;
    int mond;
};

struct screen_width {
    int start;
    int width;
    int col;
};

struct screen_height {
    int start;
    int height;
    int row;
};

struct context {
    Display * display;

    int monitors_count;
    XRRMonitorInfo * monitors;

    struct my_monitors my_mons[MONITOR_LIMIT][MONITOR_LIMIT];
    struct screen_width screen_w[MONITOR_LIMIT];
    struct screen_height screen_h[MONITOR_LIMIT];

    int total_w;
    int total_h;

    int ws;

    bool configured;

    XButtonEvent xbe;

    char data_send;
    char data_recv;

    long now_sec;
    long now_usec;
    long was_sec;
    long was_usec;

    bool do_not_send;
    int last_row;
    int last_col;
};

extern struct context ctx;

void update_ctx_now(void);
void check_usb(void);
void heartbeat(void);
void mouse_at_border_eval(void);
void set_mouse_on_message(void);
