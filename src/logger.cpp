/* SPDX-License-Identifier: BSD-3-Clause */

#include "logger.hpp"

#include <syslog.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

static const char* levels[] = {
    "Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Info", "Debug"
};

uint64_t timestamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

logger::logger(std::string const & name, int level)
    : ll(level) {

    openlog(name.c_str(), LOG_CONS, LOG_LOCAL1);
    setlogmask(LOG_UPTO(level));
}

logger::~logger() {
    closelog();
}

void logger::log(int level, std::string const & msg) {
    syslog(level, "[%s] %s", levels[level], msg.c_str());
    if (level <= ll) {
        auto ts = timestamp();
        std::cerr << ts / 1000 << "." << ts % 1000 << " [" << levels[level] << "]" << " " << msg << std::endl;
    }
}

void logger::debug(std::string const & msg) {
    log(LOG_DEBUG, msg);
}

void logger::info(std::string const & msg) {
    log(LOG_INFO, msg);
}

void logger::warn(std::string const & msg) {
    log(LOG_WARNING, msg);
}

void logger::err(std::string const & msg) {
    log(LOG_ERR, msg);
}
