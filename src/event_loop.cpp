#include "event_loop.hpp"

#include <sys/epoll.h>

#include <system_error>
#include <utility>

event_loop::event_loop(logger & log)
    : log(log)
    , epoll_fd(epoll_create1(EPOLL_CLOEXEC)) {

    if (epoll_fd < 0) {
        throw std::system_error(errno, std::system_category(), "failed to create epoll fd");
    }

}

void event_loop::run() {
    log.debug("starting event loop");

    std::array<epoll_event, 8> events;
    for (;;) {
        auto fds = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (fds < 0) {
            throw std::system_error(errno, std::system_category(), "epoll_wait failed");
        }

        for (auto i = 0; i < fds; ++i) {
            if (!fd_handlers.contains(events[i].data.fd)) {
                throw std::logic_error("epoll returned unknown fd");
            }

            fd_handlers[events[i].data.fd](EPOLLIN);
        }
    }
}

void event_loop::add_fd(int fd, callback && cb) {
    struct epoll_event ev = {};
    ev.events = EPOLLIN | EPOLLPRI;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        throw std::system_error(errno, std::system_category(), "epoll_ctl failed");
    }

    log.debug("added fd handler for fd " + std::to_string(fd));
    fd_handlers.emplace(fd, std::move(cb));
}
