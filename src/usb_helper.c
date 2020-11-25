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
#include <string.h>

#define USB_CTRL_TIMEOUT 10
#define TxUSB_CTRL_TIMEOUT 100

struct libusb_device_handle * usb_claimHIDDevice(struct libusb_device * dev, int iface) {

    if (!dev) {
        return NULL;
    }

    struct libusb_device_handle * udev = NULL;

    libusb_open(dev, &udev);
    if (!udev) {
        slog(LL_ERROR, "failed to open dev %p", dev);
        return NULL;
    }
    slog(LL_DEBUG, "opened dev %p udev %p", dev, udev);

    if (libusb_claim_interface(udev, iface) != 0) {
        libusb_close(udev);
        slog(LL_ERROR, "claim failed libusb_close 3");
        return NULL;
    } else {
        slog(LL_DEBUG, "usb_claimHIDDevice %p", udev);
    }

    return udev;
}

static unsigned int usb_get_usage_page_from_report_desc(unsigned char * buf, size_t size) {
    unsigned int data = 0xffff;

    if (!buf) {
        return 0;
    }

    for (size_t i = 0; i < size;) {
        uint8_t bsize = buf[i] & 0x03;
        if (bsize == 3) {
            bsize = 4;
        }

        uint8_t btag = buf[i] & ~0x03;  // 2 LSB bits encode length

        if (bsize) {
            data = 0;
            for (size_t j = 0; j < bsize; j++) {
                data += (buf[i + 1 + j] << (8 * j));
            }
        }

        uint8_t btype = buf[i] & (0x03 << 2);

        slog(LL_DEBUG, "size = %zu i = %zu btag = %hhu btype = %hhu bsize = %hhu and data = 0x%04x", size, i, btag,
            btype, bsize, data);
        // TODO: find usage page ff20; dont fall for ff10 etc!
        switch (btag) {
            case 0x04:               // Usage Page
                if (data == 0xFF20)  // vendor specific
                    // upage=data;
                    return data;  // DAI - need to check this routine
        }
        i += 1 + bsize;
    }

    return 0;
}

static unsigned int usb_get_usage_page_hid_device(struct libusb_device * dev,
    const struct libusb_interface_descriptor * interface, const unsigned char * buf, size_t buf_size) {

    if (!dev || !interface || !buf || buf_size < 6 || buf[1] != LIBUSB_DT_HID || buf[0] < (6U + 3U * buf[5]) ||
        buf_size < (6U + 3U * buf[5])) {

        return 0;
    }

    for (size_t i = 0; i < buf[5]; i++) {
        // we are just interested in report descriptors
        if (buf[6 + 3 * i] != LIBUSB_DT_REPORT) continue;
        unsigned int len = buf[6 + 3 * i + 1] | (buf[6 + 3 * i + 2] << 8);

        slog(LL_DEBUG, "len is %d", len);

        unsigned char dbuf[8192];
        if (len > sizeof(dbuf)) {
            continue;
        }

        slog(LL_DEBUG, "about to call open tx");

        struct libusb_device_handle * uhand = NULL;
        int ret = libusb_open(dev, &uhand);

        slog(LL_DEBUG, "libusb_open(%p, %p) returned %d and set &uhand to %p", dev, &uhand, ret, uhand);

        if (!uhand) {
            return 0;
        }

        slog(LL_DEBUG, "about to call claim i/f %hhu", interface->bInterfaceNumber);

        ret = libusb_kernel_driver_active(uhand, interface->bInterfaceNumber);

        slog(LL_DEBUG, "1 libusb_kernel_driver_active(%p, %hhu) returned %d", uhand, interface->bInterfaceNumber, ret);

        if (ret == 1) {
            ret = libusb_detach_kernel_driver(uhand, interface->bInterfaceNumber);

            slog(
                LL_DEBUG, "libusb_detach_kernel_driver(%p, %hhu) returned %d", uhand, interface->bInterfaceNumber, ret);

            if (ret == LIBUSB_ERROR_NOT_FOUND) {
                ret = libusb_detach_kernel_driver(uhand, interface->bInterfaceNumber);

                slog(LL_DEBUG, "kernel driver detach retry returned %d", ret);
            }
        }

        ret = libusb_claim_interface(uhand, interface->bInterfaceNumber);
        slog(LL_DEBUG, "XXX libusb_claim_interface(%p, %hhu) returned %d = %s", uhand, interface->bInterfaceNumber, ret,
            libusb_error_name(ret));

        if (ret == LIBUSB_ERROR_BUSY) {
            ret = libusb_claim_interface(uhand, interface->bInterfaceNumber);
            slog(LL_DEBUG, "XXX libusb_claim_interface(%p, %hhu) returned %d = %s", uhand, interface->bInterfaceNumber,
                ret, libusb_error_name(ret));
        }

        if (!ret) {
            slog(LL_INFO, "about to call control tx");

            unsigned int usage_page = 0;

            int n = libusb_control_transfer(uhand,
                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE,
                LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8), interface->bInterfaceNumber, dbuf, len,
                USB_CTRL_TIMEOUT * 3);
            if (n > 0) {
                slog(LL_DEBUG, "about to get usage page");

                usage_page = usb_get_usage_page_from_report_desc(dbuf, n);
            }

            slog(LL_DEBUG, "USB: ret = %d interface = 0x%02hhx usage_page = 0x%04x", n, interface->bInterfaceNumber,
                usage_page);

            ret = libusb_kernel_driver_active(uhand, interface->bInterfaceNumber);
            slog(LL_DEBUG, "3 libusb_kernel_driver_active(%p, %hhu) returned %d", uhand, interface->bInterfaceNumber,
                ret);

            ret = libusb_release_interface(uhand, interface->bInterfaceNumber);
            slog(LL_DEBUG, "libusb_release_interface returned %d", ret);

            ret = libusb_kernel_driver_active(uhand, interface->bInterfaceNumber);
            slog(LL_DEBUG, "4 libusb_kernel_driver_active(%p, %hhu) returned %d", uhand, interface->bInterfaceNumber,
                ret);

            libusb_close(uhand);
            slog(LL_DEBUG, "libusb_close 4");

            return usage_page;
        } else {
            libusb_close(uhand);
        }
    }

    return 0;
}

static unsigned int usb_get_usage_page(
    struct libusb_device * dev, const struct libusb_interface_descriptor * interface) {

    if (!dev || !interface) {
        return -1;
    }

    if (is_log_level(LL_DEBUG)) {
        slog(LL_DEBUG, "    bLength:            %hhu", interface->bLength);
        slog(LL_DEBUG, "    bDescriptorType:    %hhu", interface->bDescriptorType);
        slog(LL_DEBUG, "    bInterfaceNumber:   %hhu", interface->bInterfaceNumber);
        slog(LL_DEBUG, "    bAlternateSetting:  %hhu", interface->bAlternateSetting);
        slog(LL_DEBUG, "    bNumEndpoints:      %hhu", interface->bNumEndpoints);
        slog(LL_DEBUG, "    bInterfaceClass:    %hhu", interface->bInterfaceClass);
        slog(LL_DEBUG, "    bInterfaceSubClass: %hhu", interface->bInterfaceSubClass);
        slog(LL_DEBUG, "    bInterfaceProtocol: %hhu", interface->bInterfaceProtocol);
        slog(LL_DEBUG, "    iInterface:         %hhu", interface->iInterface);
        slog(LL_DEBUG, "    extra_length:       %d", interface->extra_length);
    }

    if (interface->extra_length > 0) {
        size_t size = interface->extra_length;
        const unsigned char * buf = interface->extra;
        // check through the buffer pointed to by extra for HID

        while (buf && size >= (sizeof *buf * 2) && size >= buf[0]) {
            slog(LL_DEBUG, "Size is %zu , 2 * sizeof *buf is %lu and buf[1] is %hhu", size, sizeof *buf * 2, buf[1]);

            switch (buf[1]) {
                case LIBUSB_DT_HID:
                    switch (interface->bInterfaceClass) {
                        case LIBUSB_CLASS_HID:

                            slog(LL_DEBUG, "HID - call usb_get_usage_page_hid_device");

                            return usb_get_usage_page_hid_device(dev, interface, buf, size);
                        default:
                            break;
                    }
                default:
                    break;
            }
            size -= buf[0];
            buf += buf[0];
        }
    }
    return 0;
}

int usb_readHIDDevice(
    struct libusb_device_handle * udev, int ep, int up, unsigned char * buffer, unsigned int buffer_len) {

    int actlen = 0;
    int retval = -1;
    slog(LL_DEBUG, "Interrupt Read() %p 0x%04x ep=0x%02x", udev, up, ep);

    if (udev && buffer && buffer_len > 3) {
        slog(LL_DEBUG,
            "Calling libusb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx "
            "..., Len 0x%04x Timeout %dms",
            udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], buffer_len, USB_CTRL_TIMEOUT);

        // TODO: this may hang forever (Solaris), find out why or how to do it right
        retval = libusb_interrupt_transfer(udev, ep, buffer, buffer_len, &actlen, USB_CTRL_TIMEOUT);

        slog(LL_DEBUG,
            "USB: Read via usb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx "
            "..., Buf %p Actlen 0x%04x Timeout %dms and returned %d = %s",
            udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], &buffer, actlen, USB_CTRL_TIMEOUT, retval,
            libusb_error_name(retval));

        //  Workaround spurious timeout when all data has actually been received
        if (actlen == (int)buffer_len) {
            if (retval == LIBUSB_ERROR_TIMEOUT) {
                slog(LL_DEBUG,
                    "USB: Read Timeout via usb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx "
                    "0x%02hhx 0x%02hhx ..., Buf %p Actlen 0x%04x Timeout %dms and returned %d",
                    udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], &buffer, actlen, USB_CTRL_TIMEOUT, retval);
            }
            retval = 0;
        }
    }
    return retval;
}

static void usb_detachHIDDevice(struct libusb_device * dev, int iface) {
    if (!dev) {
        return;
    }

    struct libusb_device_handle * udev = NULL;

    // detach the driver from the kernel

    int usb_open_return;
    usb_open_return = libusb_open(dev, &udev);

    // udev may expand to true, even if usb_open fails.
    if (!udev || usb_open_return) {
        return;
    }

    int err = libusb_kernel_driver_active(udev, iface);
    slog(LL_DEBUG, "In detachHID - kernel driver active returned %d = %s", err, libusb_error_name(err));

    if (err == 1) {
        libusb_set_auto_detach_kernel_driver(udev, 1);
    }

    libusb_close(udev);
}

int usb_writeHIDDevice(
    struct libusb_device_handle * udev, int ep, int up, unsigned char * buffer, unsigned int buffer_len) {

    int retval = -1;
    int transferred = 0;
    slog(LL_DEBUG, "Interrupt Write() %p 0x%04x ep=0x%02x", udev, up, ep);

    if (udev && buffer && buffer_len > 5) {
        slog(LL_DEBUG,
            "Calling usb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx "
            "0x%02hhx 0x%02hhx ..., Len 0x%04x Timeout %dms",
            udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer_len, TxUSB_CTRL_TIMEOUT);

        retval = libusb_interrupt_transfer(udev, ep, buffer, buffer_len, &transferred, TxUSB_CTRL_TIMEOUT);

        slog(LL_DEBUG, "TX Interrupt transfer returned %d and transferred %d", retval, transferred);

        if (retval == 0) {
            // Releasing interface is time-consuming, so keep it open at all times !!!
            // usb_release_interface(weydev->udev, weydev->iface);
            // usb_close(weydev->udev);
            // weydev->udev=NULL;
            slog(LL_DEBUG,
                "USB: Wrote via usb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx 0x%02hhx "
                "0x%02hhx ..., Len 0x%04x Timeout %dms",
                udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], buffer_len, TxUSB_CTRL_TIMEOUT);
        } else {
            slog(LL_DEBUG,
                "USB: ret %d = %s Failed usb_interrupt_transfer with udev: %p, ep 0x%04x, buffer 0x%02hhx 0x%02hhx "
                "0x%02hhx 0x%02hhx ..., Len 0x%04x Timeout %dms",
                retval, libusb_error_name(retval), udev, ep, buffer[0], buffer[1], buffer[2], buffer[3], buffer_len,
                TxUSB_CTRL_TIMEOUT);
        }
    }
    return retval;
}

//  The following routine needs to be sorted out to determine number of busses, usage page etc.

int usb_HidCtrlEvaluate(libusb_device * dev,
    void(add_hid)(libusb_device * dev, int iface, int ep0, int ep1, int usage_page, int vendID, int prodID)) {

    slog(LL_DEBUG, "usb_HidCtrlEvaluate: dev = %p", dev);

    if (!add_hid || !dev) {
        return -1;
    }

    // Get device descriptor for device
    struct libusb_device_descriptor desc;
    int err = libusb_get_device_descriptor(dev, &desc);
    if (err < 0) {
        slog(LL_DEBUG, "failed to get device descriptor %d = %s", err, libusb_error_name(err));
        return err;
    }

    slog(LL_DEBUG, "%04x:%04x Vendor:Product (bus %hhu, device %hhu)", (unsigned)desc.idVendor,
        (unsigned)desc.idProduct, libusb_get_bus_number(dev), libusb_get_device_address(dev));

    for (int i = 0; i < desc.bNumConfigurations; i++) {

        slog(LL_DEBUG, "i= %d and NumConf = %hhu", i, desc.bNumConfigurations);
        struct libusb_config_descriptor * config = NULL;

        err = libusb_get_active_config_descriptor(dev, &config);  // get USB config desc for currently active device
        if (err < 0 || !config) {
            slog(LL_DEBUG, "failed to get config descriptor %d = %s", err, libusb_error_name(err));
            return err;
        }

        for (int j = 0; j < desc.bNumConfigurations; j++) {
            // check each configuration

            slog(LL_DEBUG, "j= %d  Conf.NumIF = %hhu  Max pwr = %dmA  DescType is %hhu", j, config->bNumInterfaces,
                2 * config->MaxPower, config->bDescriptorType);

            for (int k = 0; k < config->bNumInterfaces; k++) {
                slog(LL_DEBUG, "k= %d", k);

                // check each interface
                const struct libusb_interface * inter = &config->interface[k];
                if (!inter) {
                    continue;
                }

                for (int l = 0; l < inter->num_altsetting; l++) {
                    // now check each alternative setting
                    const struct libusb_interface_descriptor * interdesc = &inter->altsetting[l];
                    if (!interdesc) {
                        continue;
                    }

                    if (is_log_level(LL_DEBUG)) {
                        slog(LL_DEBUG, "    bLength:            %hhu", interdesc->bLength);
                        slog(LL_DEBUG, "    bDescriptorType:    %hhu", interdesc->bDescriptorType);
                        slog(LL_DEBUG, "    bInterfaceNumber:   %hhu", interdesc->bInterfaceNumber);
                        slog(LL_DEBUG, "    bAlternateSetting:  %hhu", interdesc->bAlternateSetting);
                        slog(LL_DEBUG, "    bNumEndpoints:      %hhu", interdesc->bNumEndpoints);
                        slog(LL_DEBUG, "    bInterfaceClass:    %hhu", interdesc->bInterfaceClass);
                        slog(LL_DEBUG, "    bInterfaceSubClass: %hhu", interdesc->bInterfaceSubClass);
                        slog(LL_DEBUG, "    bInterfaceProtocol: %hhu", interdesc->bInterfaceProtocol);
                        slog(LL_DEBUG, "    iInterface:         %hhu", interdesc->iInterface);
                        slog(LL_DEBUG, "    endpoint:           %p", interdesc->endpoint);
                        slog(LL_DEBUG, "    extra:              %p", interdesc->extra);
                        slog(LL_DEBUG, "    extra_length:       %d", interdesc->extra_length);

                        const struct libusb_endpoint_descriptor * epdesc;
                        epdesc = &interdesc->endpoint[0];
                        if (epdesc) {
                            slog(LL_DEBUG, " ep.bLength:         %hhu", epdesc->bLength);
                            slog(LL_DEBUG, " ep.bDescriptorType: %hhu", epdesc->bDescriptorType);
                            slog(LL_DEBUG, " ep.bEndpointAddress:0x%02hhx", epdesc->bEndpointAddress);
                            slog(LL_DEBUG, " ep.bmAttributes:    %hhu", epdesc->bmAttributes);
                            slog(LL_DEBUG, " ep.wMaxPacketSize:  %u", (unsigned)epdesc->wMaxPacketSize);
                            slog(LL_DEBUG, " ep.bInterval:       %hhu", epdesc->bInterval);
                            slog(LL_DEBUG, " ep.bRefresh:        %hhu", epdesc->bRefresh);
                            slog(LL_DEBUG, " ep.bSynchAddress:   %hhu", epdesc->bSynchAddress);
                            slog(LL_DEBUG, " ep.extra:           %p", epdesc->extra);
                            slog(LL_DEBUG, " ep.extra_length:    %d", epdesc->extra_length);
                        }

                        if (interdesc->bNumEndpoints == 2) {
                            epdesc = &interdesc->endpoint[1];
                            if (epdesc) {
                                slog(LL_DEBUG, "Second EP    ep.bLength:         %hhu", epdesc->bLength);
                                slog(LL_DEBUG, "    ep.bDescriptorType: %hhu", epdesc->bDescriptorType);
                                slog(LL_DEBUG, "    ep.bEndpointAddress:0x%02hhx", epdesc->bEndpointAddress);
                                slog(LL_DEBUG, "    ep.bmAttributes:    %hhu", epdesc->bmAttributes);
                                slog(LL_DEBUG, "    ep.wMaxPacketSize:  %u", (unsigned)epdesc->wMaxPacketSize);
                                slog(LL_DEBUG, "    ep.bInterval:       %hhu", epdesc->bInterval);
                                slog(LL_DEBUG, "    ep.bRefresh:        %hhu", epdesc->bRefresh);
                                slog(LL_DEBUG, "    ep.bSynchAddress:   %hhu", epdesc->bSynchAddress);
                                slog(LL_DEBUG, "    ep.extra:           %p", epdesc->extra);
                                slog(LL_DEBUG, "    ep.extra_length:    %d", epdesc->extra_length);
                            }
                        }

                        slog(LL_DEBUG, "l= %d and alt is %d", l, inter->num_altsetting);
                    }

                    // check for HID
                    if (interdesc->bInterfaceClass == LIBUSB_CLASS_HID && interdesc->bInterfaceSubClass == 0 &&
                        interdesc->bInterfaceProtocol == 0) {

                        unsigned int usage_page = 0;

                        if (interdesc->bNumEndpoints == 0)  // NEED TO CHECK as 0 endpoints
                        {
                            slog(LL_DEBUG, "Usage Page call (a)");

                            usage_page = usb_get_usage_page(dev, interdesc);

                            slog(LL_DEBUG, "Usage Page (a) = 0x%04x ep = 0", usage_page);

                            add_hid(dev, interdesc->bInterfaceNumber, 0, 0, usage_page, desc.idVendor, desc.idProduct);
                        } else if (interdesc->bNumEndpoints == 2) {
                            if (is_log_level(LL_DEBUG)) {
                                slog(LL_DEBUG, "total no. of endpoints is %hhu", interdesc->bNumEndpoints);
                                slog(LL_DEBUG,
                                    "interdesc->endpoint[0].bmAttributes 0x%02hhx, LIBUSB_TRANSFER_TYPE_MASK 0x%02x "
                                    "and LIBUSB_TRANSFER_TYPE_INTERRUPT 0x%02x",
                                    interdesc->endpoint[0].bmAttributes, LIBUSB_TRANSFER_TYPE_MASK,
                                    LIBUSB_TRANSFER_TYPE_INTERRUPT);
                                slog(LL_DEBUG,
                                    "interdesc->endpoint[0].bEndpointAddress 0x%02hhx, LIBUSB_ENDPOINT_DIR_MASK 0x%02x "
                                    "and LIBUSB_ENDPOINT_IN 0x%02x",
                                    interdesc->endpoint[0].bEndpointAddress, LIBUSB_ENDPOINT_DIR_MASK,
                                    LIBUSB_ENDPOINT_IN);
                            }

                            // check for Interrupt endpoint in and out
                            if (((interdesc->endpoint[0].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) ==
                                        LIBUSB_TRANSFER_TYPE_INTERRUPT &&
                                    (interdesc->endpoint[0].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ==
                                        LIBUSB_ENDPOINT_IN) ||
                                ((interdesc->endpoint[0].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) ==
                                        LIBUSB_TRANSFER_TYPE_INTERRUPT &&
                                    (interdesc->endpoint[0].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ==
                                        LIBUSB_ENDPOINT_OUT)) {
                                // found Interrupt endpoint in so detach the driver from the kernel

                                slog(LL_DEBUG, "Usage Page call (b)");

                                // and get the usage page
                                usage_page = usb_get_usage_page(dev, interdesc);
                                slog(LL_DEBUG, "Usage Page (b) = 0x%04x ep0 = %hhu ep1 = %hhu", usage_page,
                                    interdesc->endpoint[0].bEndpointAddress, interdesc->endpoint[1].bEndpointAddress);

                                add_hid(dev, interdesc->bInterfaceNumber, interdesc->endpoint[0].bEndpointAddress,
                                    interdesc->endpoint[1].bEndpointAddress, usage_page, desc.idVendor, desc.idProduct);
                            }
                        } else if (is_log_level(LL_DEBUG)) {
                            slog(LL_DEBUG, "total no. of endpoints is %hhu", interdesc->bNumEndpoints);
                            slog(LL_DEBUG,
                                "interdesc->endpoint[0].bmAttributes 0x%02hhx, LIBUSB_TRANSFER_TYPE_MASK 0x%02x and "
                                "LIBUSB_TRANSFER_TYPE_INTERRUPT 0x%02x",
                                interdesc->endpoint[0].bmAttributes, LIBUSB_TRANSFER_TYPE_MASK,
                                LIBUSB_TRANSFER_TYPE_INTERRUPT);
                            slog(LL_DEBUG,
                                "interdesc->endpoint[0].bEndpointAddress 0x%02hhx, LIBUSB_ENDPOINT_DIR_MASK 0x%02x and "
                                "LIBUSB_ENDPOINT_IN 0x%02x",
                                interdesc->endpoint[0].bEndpointAddress, LIBUSB_ENDPOINT_DIR_MASK, LIBUSB_ENDPOINT_IN);
                        }
                        slog(LL_DEBUG, "exit else");
                        usb_detachHIDDevice(dev, interdesc->bInterfaceNumber);
                    }
                    slog(LL_DEBUG, "exit if HID");
                }
                slog(LL_DEBUG, "exit for alt");
            }
        }
    }
    return 0;
}
