/* SPDX-License-Identifier: BSD-3-Clause */

#include "usb.hpp"

#include <sys/timerfd.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <string>
#include <utility>

static const char USB_PATH[] = "/dev/bus/usb";
static const unsigned int TRANSFER_TIMEOUT = 50;

usb_dev::usb_dev(logger & log, context & ctx)
    : log(log)
    , ctx(ctx)
    , tfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {

    for (auto const & entry : std::filesystem::recursive_directory_iterator(USB_PATH)) {
        if (!entry.is_directory()) {
            struct usb_device_descriptor desc;
            ssize_t expected_size = sizeof(desc);

            auto fd = file_descriptor(::open(entry.path().c_str(), O_RDWR | O_CLOEXEC, 0));
            if (!fd.valid()) {
                log.warn("failed to open " + entry.path().string());
                continue;
            }

            auto offset = ::lseek(*fd, 0, SEEK_SET);
            if (offset == -1) {
                throw std::system_error(errno, std::system_category(), "failed to seek to usb dev descriptor");
            }

            auto size = ::read(*fd, &desc, expected_size);
            if (size == -1) {
                throw std::system_error(errno, std::system_category(), "failed to read usb dev descriptor");
            }

            if (size != expected_size) {
                throw std::runtime_error("usb dev descriptor read incomplete");
            }

            std::stringstream ss;
            ss << std::hex << "trying " << entry.path().string() << " "
               << std::setfill('0') << std::setw(4) << desc.idVendor << ":"
               << std::setfill('0') << std::setw(4) << desc.idProduct << std::dec;
            log.info(ss.str());

            if (desc.idVendor == 0x1b07 && desc.idProduct >= 0x1000 && desc.idProduct <= 0x10ff) {
                hid_fd = std::move(fd);

                detach_kernel_driver();

                unsigned int iface = 2;

                if (ioctl(*hid_fd, USBDEVFS_CLAIMINTERFACE, &iface) < 0) {
                    throw std::system_error(errno, std::system_category(), "failed to claim hid dev");
                }
                heartbeat();

                break;
            }
        }
    }

    if (!hid_fd.valid()) {
        throw std::runtime_error("wey usb device not found");
    }

    struct itimerspec ts = {};
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 10 * 1000;
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 10 * 1000;

    if (timerfd_settime(*tfd, 0, &ts, NULL) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to arm timer");
    }

    ctx.get_el().add_fd(*tfd, std::bind(&usb_dev::handle_events, this, std::placeholders::_1));
    read_mouse_pos();
}

void usb_dev::submit_transfer(transfer_t & t) {
    log.debug("submitting transfer " + std::to_string(t.id));
    auto urb = &t.urb;
    urb->usercontext = &t.id;
    urb->type = USBDEVFS_URB_TYPE_INTERRUPT;
    urb->endpoint = 0x03 | USB_DIR_IN;
    urb->buffer = t.buf.data();
    urb->buffer_length = t.buf.size();

    if (ioctl(*hid_fd, USBDEVFS_SUBMITURB, urb) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to submit urb");
    }
    log.debug("done");
}

void usb_dev::handle_events(int) {
    while (reap()) { }

    if (transfers.empty()) {
        read_mouse_pos();
    }
}

bool usb_dev::reap() {
    struct usbdevfs_urb * urb = nullptr;
    auto ret = ioctl(*hid_fd, USBDEVFS_REAPURBNDELAY, &urb);

    if (ret < 0) {
        if (errno != EAGAIN) {
            throw std::system_error(errno, std::system_category(), "usbdevfs reap failed");
        }
        return false;
    }

    if (urb->status != 0) {
        throw std::system_error(-urb->status, std::system_category(), "unhandled urb status");
    }

    if (urb->endpoint == (0x03 | USB_DIR_IN)) {
        auto buf = static_cast<uint8_t*>(urb->buffer);
        std::stringstream ss("wey packet: ");
        for (auto i = 0; i < urb->actual_length; ++i) {
            uint8_t b = static_cast<uint8_t*>(buf)[i];
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
        }
        log.debug(ss.str());

        if (buf[0] == 0x05 && buf[1] == 0x00) {
            last_sent_pos.reset();
            ctx.set_mouse_pos({
                .screen = buf[2],
                .border = buf[3],
                .pos = static_cast<uint16_t>((static_cast<uint16_t>(buf[5]) << 8) | static_cast<uint16_t>(buf[4]))
            });
            done();
        }
    } else {
        log.debug("send completed");
    }

    auto id = *static_cast<uint64_t*>(urb->usercontext);
    transfers.erase(std::remove_if(transfers.begin(), transfers.end(), [&](auto & t) { return t.id == id; }),
        transfers.end());

    return true;
}

void usb_dev::heartbeat() {
    log.debug("sending heartbeat");
    std::array<uint8_t, 64> buf {};
    buf[0] = 0x05;

    struct usbdevfs_bulktransfer data {
        .ep = 3,
        .len = buf.size(),
        .timeout = TRANSFER_TIMEOUT,
        .data = buf.data()
    };

    if (ioctl(*hid_fd, USBDEVFS_BULK, &data) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to send heartbeat");
    }
}

void usb_dev::read_mouse_pos() {
    auto & t = transfers.emplace_back(transfer_t{});
    t.id = last_id++;
    submit_transfer(t);
}

void usb_dev::send_mouse_pos(mouse_pos_t const & mp) {
    if (last_sent_pos.has_value() && (*last_sent_pos).screen == mp.screen && (*last_sent_pos).border == mp.border) {
        return;
    }

    last_sent_pos = mp;

    std::array<uint8_t, 64> buf {};
    buf[0] = 0x05;
    buf[1] = 0x01;
    buf[2] = mp.screen;
    buf[3] = mp.border;
    buf[4] = mp.pos & 0x00ff;
    buf[5] = mp.pos >> 8;

    struct usbdevfs_bulktransfer data {
        .ep = 3,
        .len = buf.size(),
        .timeout = TRANSFER_TIMEOUT,
        .data = buf.data()
    };

    if (ioctl(*hid_fd, USBDEVFS_BULK, &data) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to send done");
    }

    done();
}

void usb_dev::done() {
    std::array<uint8_t, 64> buf {};
    buf[0] = 0x05;
    buf[1] = 0x02;

    struct usbdevfs_bulktransfer data {
        .ep = 3,
        .len = buf.size(),
        .timeout = TRANSFER_TIMEOUT,
        .data = buf.data()
    };

    if (ioctl(*hid_fd, USBDEVFS_BULK, &data) < 0) {
        throw std::system_error(errno, std::system_category(), "failed to send done");
    }
}

void usb_dev::detach_kernel_driver() {
    struct usbdevfs_getdriver getdrv = { };
    getdrv.interface = 2;

    if (ioctl(*hid_fd, USBDEVFS_GETDRIVER, &getdrv) < 0) {
        log.debug("usbdevfs getdriver failed, should be fine since we can claim the interface now");
        return;
    }

    if (std::string(getdrv.driver) != "usbfs") {
        log.debug("usbfs not attached, current driver: " + std::string(getdrv.driver));
        ioctl(*hid_fd, USBDEVFS_RESET, NULL);
    }

    struct usbdevfs_ioctl command {
        .ifno = 2,
        .ioctl_code = USBDEVFS_DISCONNECT,
        .data = NULL
    };

    if (ioctl(*hid_fd, USBDEVFS_IOCTL, &command) < 0) {
        log.debug("failed to detach kernel driver");
    }
}
