#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <termios.h>
#include <poll.h>

#include "usb.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

libusb_device_handle *handle;

static void usage(int argc, char **argv)
{
    fprintf(stderr, "usage: %s\n", argv[0]);

    exit(1);
}

static int usb_match_func(usb_ifc_info *ifc)
{
    if (ifc->dev_vendor != 0x9999)
        return -1;
    if (ifc->dev_product != 0x9999)
        return -1;

    return 0; /* interface to open */
}

int main(int argc, char **argv)
{
    int status;
    int pollcount;

    if (argc < 1) {
        fprintf(stderr, "not enough arguments\n");
        usage(argc, argv);
    }

    handle = usb_open(&usb_match_func);
    if (!handle) {
        fprintf(stderr, "couldn't open device\n");
        return 1;
    }

    printf("got handle %p\n", handle);

#if 0
    static char buf[4096];
    int ret = usb_read_sync(handle, 0x1, buf, sizeof(buf), 1000);
    printf("usb_read_sync returns %d\n", ret);
#endif
#if 1
    static char buf[4095];

    for (int i = 0; i < sizeof(buf); i++)
        buf[i] = i;

    int ret = usb_write_sync(handle, 0x1, buf, sizeof(buf), 5000);
    printf("usb_write_sync returns %d\n", ret);
    ret = usb_write_sync(handle, 0x1, buf, sizeof(buf), 5000);
    printf("usb_write_sync returns %d\n", ret);
#endif
    usb_close(handle);

    return 0;
}

