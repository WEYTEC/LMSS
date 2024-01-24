#pragma once

#include <functional>
#include <unordered_map>

#include "logger.hpp"

class event_loop final {
public:
    using callback = std::function<void(int ev)>;

    event_loop(logger &);
    void run();
    void add_fd(int fd, callback && cb);

private:
    logger & log;
    int epoll_fd = -1;
    std::unordered_map<int, callback> fd_handlers;
};
