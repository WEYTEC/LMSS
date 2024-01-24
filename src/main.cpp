#include <syslog.h>

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "lmss.hpp"
// #include "display.hpp"
// #include "event_loop.hpp"
#include "logger.hpp"
// #include "usb.hpp"

int main(int argc, char* argv[]) {
    int log_level = LOG_ERR;
    int c = 0;

    while ((c = getopt(argc, argv, "v::")) != -1) {
        switch (c) {
            case 'v':
                log_level = std::stoi(optarg);
                if (log_level < 0 || log_level > 7) {
                    std::cout << "invalid log level (0-7)" << std::endl;
                    return -1;
                }
                break;
            case '?':
                if (optopt == 'v') {
                    std::cout << "Option -" << optopt << " requires an argument." << std::endl;
                } else {
                    std::cout << "invalid option" << std::endl;
                }
                return -1;
            default:
                return -1;
        }
    }

    logger log("LMSS", log_level);

    try {
        lmss l(log);
        l.run();
        // event_loop el(log);
        // log.info("connecting usb...");
        // usb_dev usb(log, el);
        // display dsp(log, el, usb);

        // el.run();


        // auto last_heartbeat = std::chrono::steady_clock::now();

        // while (true) {
        //     auto now = std::chrono::steady_clock::now();
        //     dsp.handle_events();

        //     auto mp = usb.read_mouse_pos();
        //     if (mp.has_value()) {
        //         dsp.set_mouse_pos(mp.value());
        //     }

        //     if (now - last_heartbeat >= std::chrono::seconds(1)) {
        //         last_heartbeat = now;
        //         usb.heartbeat();
        //     }
        //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // }
    } catch (std::runtime_error const & e) {
        log.err(e.what());
        return -1;
    }

    return 0;
}
