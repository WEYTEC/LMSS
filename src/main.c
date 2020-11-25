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
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

int setup(int argc, char ** argv) {
    // check command line options
    char * vvalue = NULL;
    int c = 0;
    int vflag = 0;

    while ((c = getopt(argc, argv, "v::")) != -1) {
        switch (c) {
            case 'v':
                vflag = 1;
                vvalue = optarg;
                break;
            case '?':
                if (optopt == 'v')
                    slog(LL_ERROR, "Option -%c requires an argument.", optopt);
                else if (isprint(optopt))
                    slog(LL_ERROR, "Unknown option '-%c'", optopt);
                else
                    slog(LL_ERROR, "Unknown option character '\\x%x'.", optopt);
                return 1;
            default:
                return 1;
        }
    }

    for (int index = optind; index < argc; index++) {
        slog(LL_ERROR, "Non-option argument %s", argv[index]);
        return 1;
    }

    if (vflag && vvalue) {
        if ((vvalue[0] - '0') >= 0 && (vvalue[0] - '0') <= 4) {
            int llvl = (int)vvalue[0] - '0';
            set_log_level(llvl);
            slog(LL_INFO, "Started with logging level %d", llvl);
        } else {
            slog(LL_ERROR,
                "Loglevel not supported, "
                "available loglevels are 0 (OFF), 1 (ERROR), 2 (CRITICAL), 3 (INFO), 4 (DEBUGG)");
            return 1;
        }
    }

    // check user rights
    uid_t notrootuser = geteuid();

    if (notrootuser) {
        slog(LL_ERROR, "Not running as root");
        return -1;
    }

    slog(LL_DEBUG, "Running as root");

    // set start values
    ctx.configured = false;
    ctx.do_not_send = false;
    ctx.data_send = 0;
    ctx.data_recv = 1;

    // open display
    char * dispvar = getenv("DISPLAY");
    slog(LL_INFO, "DISPLAY is %s", dispvar ? dispvar : "NULL");

    update_ctx_now();

    ctx.display = XOpenDisplay(dispvar);

    if (!ctx.display) {
        slog(LL_ERROR, "Unable to open the display");
        return -1;
    }

    gen_smart_touch_screens();

    update_ctx_now();
    slog(LL_INFO, "XOpenDisplay(\"%s\") =  %p", dispvar ? dispvar : "NULL", ctx.display);
    return 0;
}

void cleanup() {
    // ToDo: when removing the endless loop this incomplete cleanup must be extended
    XCloseDisplay(ctx.display);
}

/**
 * MAIN / EXECUTE
 *
 * Loop and check every 50ms to see if any USB devices have been added or removed,
 * send switching data if boundary hit and position mouse if requested to do so.
 */
int main(int argc, char ** argv) {

    if (setup(argc, argv)) {
        return EXIT_FAILURE;
    }

    for (; true; ) {
        usleep(50 * 1000);
        update_ctx_now();

        if (ctx.now_usec > ((ctx.was_usec + 5000) % 1000000)) {
            ctx.data_recv = 0;
        } else {
            ctx.data_recv = 1;
        }

        check_usb();
        heartbeat();
        mouse_at_border_eval();
        set_mouse_on_message();

        ctx.was_sec = ctx.now_sec;
        ctx.was_usec = ctx.now_usec;
    }

    cleanup();

    return EXIT_SUCCESS;
}
