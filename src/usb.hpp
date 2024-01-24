#pragma once

#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include "context.hpp"
#include "logger.hpp"
#include "types.hpp"


class usb_dev final {
public:
    usb_dev(logger &, context &);

    void heartbeat();
    void send_mouse_pos(mouse_pos_t const &);

private:
    struct transfer_t {
        uint64_t id;
        std::array<uint8_t, 64> buf;
        usbdevfs_urb urb;
    };

    void handle_events(int);
    void done();
    void detach_kernel_driver();

    void read_mouse_pos();
    void submit_transfer(transfer_t &);
    bool reap();

    uint64_t last_id = 0;
    std::vector<transfer_t> transfers;
    logger & log;
    context & ctx;
    int hid_fd = -1;
    int tfd = -1;
};
