/* SPDX-License-Identifier: BSD-3-Clause */

#include "file_descriptor.hpp"

#include <unistd.h>

#include <algorithm>
#include <utility>

file_descriptor::file_descriptor(int fd)
    : fd(fd) { }

file_descriptor::~file_descriptor() {
    if (valid()) {
        close();
    }
}

file_descriptor& file_descriptor::operator=(file_descriptor&& other) {
    if (valid()) {
        close();
    }

    std::swap(fd, other.fd);
    return *this;
}

void file_descriptor::close() {
    ::close(fd);
    fd = -1;
}
