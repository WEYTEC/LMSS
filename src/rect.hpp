#pragma once

#include <stdint.h>

struct rect {
    rect(int x, int y, int w, int h, int screen, uint8_t border)
        : x(x), y(y), w(w), h(h), screen(screen), border(border) { }

    bool inside(int px, int py) {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }

    int x;
    int y;
    int w;
    int h;
    int screen;
    uint8_t border;
};
