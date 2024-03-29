/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include <functional>
#include <unordered_map>

#include "file_descriptor.hpp"
#include "logger.hpp"

class event_loop final {
public:
    using callback = std::function<void(int ev)>;

    event_loop(logger &);
    void run();
    void add_fd(int fd, callback && cb);

private:
    logger & log;
    file_descriptor epoll_fd;
    std::unordered_map<int, callback> fd_handlers;
};
