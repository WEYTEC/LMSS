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

#include "utilities.h"
#include <assert.h>

static void merge(int * const begin, int * const mid, int * const end) {
    int buffer[end - begin];
    int * temp = buffer;
    int * left = begin;
    int * right = mid;

    while (left < mid && right < end) {
        if (*left <= *right) {
            *temp++ = *left++;
        } else {
            *temp++ = *right++;
        }
    }
    while (left < mid) {
        *temp++ = *left++;
    }
    while (right < end) {
        *temp++ = *right++;
    }

    temp = buffer;
    left = begin;
    while (left < end) {
        *left++ = *temp++;
    }
}

void merge_sort(int * const begin, int * const end) {
    if (begin && end && (begin + 1) < end) {
        int * const mid = begin + (end - begin) / 2u;
        merge_sort(begin, mid);
        merge_sort(mid, end);
        merge(begin, mid, end);
    }
}

int * compact(int * const begin, int * const end) {
    int * read = begin + 1;
    int * write = begin;
    int * retval = end;

    assert(begin <= end);
    if (begin && end && (read) < end) {
        while (read < end) {
            if (*read != *write) {
                *++write = *read++;
            } else {
                read++;
            }
        }
        retval = ++write;
        while (write < end) {
            *write++ = 0xFFFF;
        }
    }
    return retval;
}
