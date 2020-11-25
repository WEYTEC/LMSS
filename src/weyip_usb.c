/*
Mouse Switching Software
for Linux and Unix based operating systems

BSD 3-Clause License

Copyright (c) 2020, WEY Technology AG
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "log.h"
#include "usb_helper.h"
#include "weyip_usb.h"
#include <stddef.h>
#include <unistd.h>

#define USB_BUFFER_SIZE 64U
#define WEY_DEVICES_MAX 32U

static struct wey_usb_devices {
    struct libusb_device * dev;
    struct libusb_device_handle * udev;
    int iface;
    int ep_out;
    int ep_in;
    int up;
    int vid;
    int pid;
} wey_devices[WEY_DEVICES_MAX];

static size_t wey_devices_index = 0;

static void wey_devices_init() {
    for (size_t i = 0; i < WEY_DEVICES_MAX; i++) {
        if (i < wey_devices_index && wey_devices[i].udev) {
            slog(LL_DEBUG, "wey_devices[%d].iface = %d udev= *%p*", i, wey_devices[i].iface, wey_devices[i].udev);

            int err = libusb_release_interface(wey_devices[i].udev, wey_devices[i].iface);
            libusb_close(wey_devices[i].udev);
            slog(LL_INFO, "libusb_close 1 udev=*%p* release returned %d", wey_devices[i].udev, err);
        }
        wey_devices[i].udev = NULL;
        wey_devices[i].dev = NULL;
    }
    wey_devices_index = 0;
}

static void wey_devices_add(libusb_device * dev, int iface, int ep0, int ep1, int usage_page, int vendID, int prodID) {
    slog(LL_DEBUG,
        "wey_devices_add: Vendor ID is 0x%04x, Product ID is 0x%04x, dev is %p, iface is %d, ep0 is %d , ep1 is %d and "
        "Usage Page is 0x%04x",
        vendID, prodID, dev, iface, ep0, ep1, usage_page);

    // sort endpoints
    if (ep1 < ep0) {
        int tmp = ep0;
        ep0 = ep1;
        ep1 = tmp;
    }

    // WEY Device : VendorID = 0x1B07 - ProductID 0x0100
    if (dev && vendID == 0x1B07 && prodID >= 0x01000 && prodID <= 0x010FF &&
        ((!ep0 && !ep1) || (ep0 < LIBUSB_ENDPOINT_IN && ep1 >= LIBUSB_ENDPOINT_IN))) {

        slog(LL_DEBUG, "WEYIP device and usage page is 0x%04x", usage_page);

        if (usage_page != 0xFF20 || wey_devices_index >= WEY_DEVICES_MAX) {  // usage page for mouse switch
            return;
        }

        wey_devices[wey_devices_index].dev = dev;
        wey_devices[wey_devices_index].udev = usb_claimHIDDevice(dev, iface);
        wey_devices[wey_devices_index].iface = iface;
        wey_devices[wey_devices_index].ep_out = ep0;
        wey_devices[wey_devices_index].ep_in = ep1;
        wey_devices[wey_devices_index].up = usage_page;
        wey_devices[wey_devices_index].vid = vendID;
        wey_devices[wey_devices_index].pid = prodID;

        slog(LL_DEBUG,
            "USB-Device-Add (Interrupt-%02i): VId=0x%04x - PId=0x%04x - Iface=0x%02x - Endpoint-out=0x%02x - "
            "Endpoint-in=0x%02x - UsagePage=0x%04x - udev=%p",
            wey_devices_index, vendID, prodID, iface, ep0, ep1, usage_page, wey_devices[wey_devices_index].udev);

        ++wey_devices_index;
    }
}

int enumerate_weyip_usb(void) {
    static int usb_device_count = 0;
    static bool first_run = true;

    if (first_run) {
        int err = libusb_init(NULL);  // initial call to the library
        if (err) {
            return -1;
        }
        int ll = is_log_level(LL_DEBUG) ? LIBUSB_LOG_LEVEL_DEBUG
                                        : is_log_level(LL_INFO)
                ? LIBUSB_LOG_LEVEL_INFO
                : is_log_level(LL_WARNING) ? LIBUSB_LOG_LEVEL_WARNING
                                           : is_log_level(LL_ERROR) ? LIBUSB_LOG_LEVEL_ERROR : LIBUSB_LOG_LEVEL_NONE;
        libusb_set_debug(NULL, ll);  // set debug level accordingly

        slog(LL_INFO, "HID Enumerate");
        wey_devices_init();
        first_run = false;
    }

    libusb_device ** devs = NULL;  // array of pointers, NULL terminated by libusb_get_device_list
    ssize_t count =
        libusb_get_device_list(NULL, &devs);  // returns the number of USB devices and devs holds the list of devices

    if (count < 0 || devs == NULL) {
        slog(LL_DEBUG, "failed to retrieve USB devices: %ld = %s", count, libusb_error_name(count));
        return -1;
    }
    slog(LL_DEBUG, "USB devices %ld and devs is %p", count, devs);

    if (count != usb_device_count) {  // Yes, there was a change

        slog(LL_INFO, "USB Devices (added/removed) ***** usb_device_count is ** %ld **", count);

        usb_device_count = count;

        // check if NOT (WEY device was there and still there) THEN call wey_devices_init
        slog(LL_DEBUG, "*****wey_devices[0].dev is ** %p **", wey_devices[0].dev);

        if (wey_devices_index > 0 && wey_devices[0].dev) {
            struct libusb_device_descriptor devicedesc;
            int err = libusb_get_device_descriptor(wey_devices[0].dev, &devicedesc);
            slog(LL_DEBUG, "libusb_get_device_descriptor returned %d = %s", err, libusb_error_name(err));

            if (!err) {
                return 0;
            }
        }
        wey_devices_init();  // call to initialise (clearout current) WEY devices (array of 32 devices)

        usleep(5000 * 1000);  // this somehow works, but is out of spec: max waiting time is one second

        for (libusb_device ** pdev = devs; *pdev != NULL; pdev++) {
            if (usb_HidCtrlEvaluate(*pdev, wey_devices_add)) {
                break;  // finish on fault
            }
        }
    }

    libusb_free_device_list(devs, true);
    return 0;
}

int read_mouse_position(uint8_t * screen, uint8_t * border, uint16_t * position) {
    // Read Mouse Switch Command from all Devices in List
    // (usually only one device)

    /*
      // ToDo: fix solaris workarounds
      ATM usb polling is broken under solaris (tested on 11.4)
      it will work ~5x and then get stuck. Needs further investigation.
      So solaris will loose height information on monitor switching for now.
    */
#ifndef SOLARIS_WORKAROUND

    uint8_t buffer[USB_BUFFER_SIZE] = {0};

    for (size_t i = 0; i < wey_devices_index && wey_devices[i].dev != NULL; i++) {

        // ToDo: get an idea why we loop here and why twice, old structure used variable loop count with high
        //       values for old not WEYIP devices
        for (int n = 0; n < 2; n++) {
            slog(LL_DEBUG, "Loop %d", n);

            int err = usb_readHIDDevice(
                wey_devices[i].udev, wey_devices[i].ep_in, wey_devices[i].up, buffer, USB_BUFFER_SIZE);

            if (!err) {
                slog(LL_DEBUG,
                    "Read In buffer[0] .. buffer[7] 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx "
                    "0x%02hhx",
                    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

                if (buffer[0] == 0x05 && buffer[1] == 0) {
                    if (screen) {
                        *screen = buffer[2];
                    }
                    if (border) {
                        *border = buffer[3];
                    }
                    if (position) {
                        *position = (uint16_t)buffer[5] << 8 | (uint16_t)buffer[4];
                    }

                    return 0;  // will only read first device
                }
            }

            if (err == -4) {
                slog(LL_DEBUG, "clearing WEY devices");

                wey_devices_init();
                return -4;
            }
        }
        slog(LL_DEBUG, "usb_readHIDDevice returned no data");
    }
#endif
    return -1;
}

int send_heartbeat(void) {
    uint8_t buffer[USB_BUFFER_SIZE] = {0};
    buffer[0] = 0x05;  // Report ID
    // buffer[1] = 0x00;  // Command

    // Send Mouse Switch Command to all Devices in List
    // (usually only one device)
    for (size_t i = 0; i < wey_devices_index && wey_devices[i].dev != NULL; i++) {
        // This will terminate when first send complete!!!!
        int err =
            usb_writeHIDDevice(wey_devices[i].udev, wey_devices[i].ep_out, wey_devices[i].up, buffer, USB_BUFFER_SIZE);
        if (err == -4) {
            slog(LL_DEBUG, "clearing WEY devices");
            slog(LL_DEBUG, "TX*******************************");

            wey_devices_init();
        }
        return err;
    }
    slog(LL_WARNING, "No WEYIP device found");
    return -1;
}

int send_mouse_position(uint8_t screen, uint8_t border, uint16_t position) {
    uint8_t buffer[USB_BUFFER_SIZE] = {0};
    buffer[0] = 0x05;  // Report ID
    buffer[1] = 0x01;  // Command
    buffer[2] = screen;
    buffer[3] = border;
    buffer[4] = position & 0x00FF;
    buffer[5] = position >> 8;

    // Send Mouse Switch Command to all Devices in List
    // (usually only one device)
    for (size_t i = 0; i < wey_devices_index && wey_devices[i].dev != NULL; i++) {
        // This will terminate when first send complete!!!!
        return usb_writeHIDDevice(
            wey_devices[i].udev, wey_devices[i].ep_out, wey_devices[i].up, buffer, USB_BUFFER_SIZE);
    }
    slog(LL_WARNING, "No WEYIP device found");
    return -1;
}

int send_mouse_positioning_done(void) {
    uint8_t buffer[USB_BUFFER_SIZE] = {0};
    buffer[0] = 0x05;  // Report ID
    buffer[1] = 0x02;  // Command

    // Send Mouse Switch Command to all Devices in List
    // (usually only one device)
    for (size_t i = 0; i < wey_devices_index && wey_devices[i].dev != NULL; i++) {
        // This will terminate when first send complete!!!!
        return usb_writeHIDDevice(
            wey_devices[i].udev, wey_devices[i].ep_out, wey_devices[i].up, buffer, USB_BUFFER_SIZE);
    }
    slog(LL_WARNING, "No WEYIP device found");
    return -1;
}
