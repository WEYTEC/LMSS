#include "lmss.hpp"

#include <sys/timerfd.h>

lmss::lmss(logger & log)
    : log(log)
    , el(log)
    , usb(log, *this)
    , dsp(log, *this)
    , tfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {

    if (tfd < 0) {
        throw std::runtime_error("invalid timer fd");
    }

    struct itimerspec ts = {};
    ts.it_interval.tv_sec = 1;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 1;
    ts.it_value.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &ts, NULL) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to arm timer");
    }

    el.add_fd(tfd, std::bind(&lmss::heartbeat, this, std::placeholders::_1));
}

void lmss::heartbeat(int) {
    uint64_t count;
    if (__builtin_expect(::read(tfd, &count, sizeof(count)), sizeof(count)) < 0) {
        if (errno != EAGAIN) {
            throw std::system_error(errno, std::system_category(), "failed to read timer");
        }
    }
    usb.heartbeat();
}

void lmss::set_mouse_pos(mouse_pos_t const & mp) {
    dsp.set_mouse_pos(mp);
}

void lmss::mouse_at_border(mouse_pos_t const & mp) {
    usb.send_mouse_pos(mp);
}
