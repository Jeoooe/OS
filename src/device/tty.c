#include <device/dev.h>
#include <console.h>
/*
 *终端虚拟设备
 */


static int tty_read(void* dev, void *buf, size_t count, uint32_t index, int flag) {
    device_read(device_find(DEV_KEYBOARD, 0)->dev, buf, 1, 0, 0);
    return 0;
}

static int tty_write(void* dev, void *buf, size_t count, uint32_t index, int flag) {
    console_write((char*)buf, count);
    return 0;
}

static int tty_ioctl(void* dev, int cmd, void *args, int flag) {
    return 0;
}

void tty_init() {
    device_install("tty0", 0, DEV_CHAR, DEV_TTY, NULL, tty_ioctl, tty_read, tty_write);
}

