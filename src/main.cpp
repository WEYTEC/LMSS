/* SPDX-License-Identifier: BSD-3-Clause */

#include <iostream>
#include <thread>

#include "argparse.hpp"
#include "config.hpp"
#include "lmss.hpp"
#include "logger.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser app("lmss", VERSION, argparse::default_arguments::help);
    app.add_argument("-v")
        .default_value(3)
        .choices(0, 1, 2, 3, 4, 5, 6, 7)
        .metavar("LEVEL")
        .nargs(1)
        .scan<'i', int>()
        .help("set log verbosity (0-7)");
    app.add_argument("-V", "--version")
        .default_value(false)
        .implicit_value(true)
        .help("print lmss version");

    try {
        app.parse_args(argc, argv);
    } catch (std::exception const & e) {
        std::cerr << e.what() << std::endl;
        std::cerr << app;
        std::exit(1);
    }

    if (app.get<bool>("-V")) {
        std::cout << "lmss version " << VERSION << std::endl;
        return 0;
    }

    auto log_level = app.get<int>("-v");

    logger log("LMSS", log_level);

    while (true) {
        try {
            lmss l(log);
            l.run();
        } catch (std::runtime_error const & e) {
            log.err(e.what());
            log.warn("reinitializing");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
