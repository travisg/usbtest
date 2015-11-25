/*
 * Copyright (C) 2012 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "usb.h"
#include <libusb-1.0/libusb.h>

static int libusb_initted = 0;
static libusb_context *ctx;

static libusb_device_handle *usb_open_or_list(ifc_match_func callback, int do_open)
{
    int err;
    libusb_device_handle *dhandle = NULL;

    if (!libusb_initted) {
        if (libusb_init(&ctx) < 0)
            return NULL;

        libusb_initted = 1;
    }

//  libusb_set_debug(ctx, 100);

    libusb_device **list;
    ssize_t len = libusb_get_device_list(ctx, &list);
//  printf("len %d, list %p\n", len, list);
    if (len < 0)
        return NULL;

    int i;
    for (i = 0; i < len; i++) {
        struct libusb_device_descriptor desc;
        struct usb_ifc_info info;

//      printf("list entry %p\n", list[i]);
        if (libusb_get_device_descriptor(list[i], &desc) < 0)
            continue;

        libusb_device *dev = libusb_ref_device(list[i]);

        // XXX fill it in better, but for now the callback only wants vid/pid
        memset(&info, 0, sizeof(info));
        info.dev_vendor = desc.idVendor;
        info.dev_product = desc.idProduct;
        sprintf( info.address, "%u:%u", (unsigned int)libusb_get_bus_number(dev), (unsigned int)libusb_get_device_address(dev) );

        if ( desc.iSerialNumber != 0 )
        {
            dhandle = NULL;

            if ( libusb_open(dev, &dhandle) == LIBUSB_SUCCESS)
            {
                libusb_get_string_descriptor_ascii(dhandle, desc.iSerialNumber, (unsigned char*)info.serial_number, sizeof(info.serial_number));

                libusb_close(dhandle);

                dhandle = NULL;
            }
        }

        libusb_unref_device( dev );

//      printf("calling callback: 0x%x, 0x%x\n", desc.idVendor, desc.idProduct);
        int ifc_to_open = callback(&info);
        if (ifc_to_open >= 0) {
//          printf("found device\n");
            if (do_open)
            {
                dev = libusb_ref_device(list[i]);

                if (libusb_open(dev, &dhandle) < 0) {
                    goto err;
                }

                err = libusb_set_configuration(dhandle, 1);
                printf("libusb_set_config returns %d\n", err);

                err = libusb_claim_interface(dhandle, ifc_to_open);
                //printf("libusb_claim returns %d\n", err);
                if (err < 0) {
                    printf("failed to claim interface\n");
                    libusb_close(dhandle);
                    goto err;
                }

                //printf("got device, opened\n");

                break;
            }
            else
            {
                const char *dev_serial_number = info.serial_number;
                if ( dev_serial_number[0] == 0 )
                    dev_serial_number = "(null)";
                printf("serial number: %s (address = %s)\n", dev_serial_number, info.address);
            }
        }
    }

    libusb_free_device_list(list, 1);

    return dhandle;

err:
    libusb_free_device_list(list, 0);
    return NULL;
}

libusb_device_handle *usb_open(ifc_match_func callback)
{
    return usb_open_or_list(callback, 1 );
}

void usb_list_devices(ifc_match_func callback)
{
    (void)usb_open_or_list(callback, 0 );
}

int usb_close(libusb_device_handle *h)
{
    libusb_close(h);
    return 0;
}

int usb_read_sync(libusb_device_handle *h, unsigned char ep, void *_data, int len, int timeout)
{
    //printf("read: %p, ep 0x%x, data %p, len %d\n", h, ep, _data, len);

    int actual = len;
    int err = libusb_bulk_transfer(h, 0x80 | ep, _data, len, &actual, timeout);
    printf("read returns %d, actual %d\n", err, actual);
    if (err < 0)
        return -EIO;

    return actual;
}

int usb_write_sync(libusb_device_handle *h, unsigned char ep, const void *_data, int len, int timeout)
{
    //printf("write: %p, ep 0x%x, data %p, len %d\n", h, ep, _data, len);

    int actual = len;
    int err = libusb_bulk_transfer(h, ep, (void *)_data, len, &actual, timeout);
    printf("write returns %d, actual %d\n", err, actual);
    if (err < 0)
        return -EIO;

    return actual;
}

int usb_read_async(libusb_device_handle *h, unsigned char ep, void *_data, int len, libusb_transfer_cb_fn cb, void *cbargs)
{
    struct libusb_transfer *t;

    t = libusb_alloc_transfer(0);
    if (!t)
        return -ENOMEM;

    libusb_fill_bulk_transfer(t, h, 0x80 | ep, _data, len, cb, cbargs, 5000);

    if (libusb_submit_transfer(t) < 0) {
        fprintf(stderr, "error submitting transfer\n");
        libusb_free_transfer(t);

        return -EIO;
    }

    return 0;
}

int usb_write_async(libusb_device_handle *h, unsigned char ep, const void *_data, int len, libusb_transfer_cb_fn cb, void *cbargs)
{
    struct libusb_transfer *t;

    t = libusb_alloc_transfer(0);
    if (!t)
        return -ENOMEM;

    libusb_fill_bulk_transfer(t, h, ep, (void *)_data, len, cb, cbargs, 5000);

    if (libusb_submit_transfer(t) < 0) {
        fprintf(stderr, "error submitting transfer\n");
        libusb_free_transfer(t);

        return -EIO;
    }

    return 0;
}

void usb_do_work(void)
{
    struct timeval zero_tv;

    zero_tv.tv_sec = 0;
    zero_tv.tv_usec = 0;

    libusb_handle_events_timeout(ctx, &zero_tv);
}

const struct libusb_pollfd **usb_get_pollfds(void)
{
    return libusb_get_pollfds(ctx);
}

