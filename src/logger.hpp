/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include <string>


class logger final {
public:
    logger(std::string const & name, int level);
    ~logger();

    void debug(std::string const &);
    void info(std::string const &);
    void warn(std::string const &);
    void err(std::string const &);

private:
    void log(int level, std::string const &);
    int ll;
};
