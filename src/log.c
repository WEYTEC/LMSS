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

#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

static enum loglevel loglevel = LL_WARNING;

bool is_log_level(enum loglevel loglvl) {
    return loglvl <= loglevel;
}

void set_log_level(enum loglevel loglvl) {
    loglevel = loglvl;
}

void slog(enum loglevel loglvl, char msg[], ...) {

    if (loglvl > loglevel && loglvl != LL_ERROR) {
        return;
    }

    va_list arguments;
    va_start(arguments, msg);

    if (loglvl == LL_ERROR) {
        va_list tmp;
        va_copy(tmp, arguments);
        vfprintf(stderr, msg, tmp);
        fprintf(stderr, "\n");
        va_end(tmp);
    }

    if (loglvl <= loglevel) {
        int loglvltxt;
        switch (loglvl) {
            case LL_ERROR:
                loglvltxt = LOG_ERR;
                break;

            case LL_WARNING:
                loglvltxt = LOG_WARNING;
                break;

            case LL_INFO:
                loglvltxt = LOG_INFO;
                break;

            case LL_DEBUG:
                loglvltxt = LOG_DEBUG;
                break;

            default:
                loglvltxt = LOG_ERR;
        }

        // do not spam syslog with all those debug messages
        if (loglevel != LL_DEBUG) {
            vsyslog(loglvltxt, msg, arguments);
        } else {
            vprintf(msg, arguments);
            printf("\n");
        }
    }

    va_end(arguments);
}
