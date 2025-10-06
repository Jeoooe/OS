#include <device/dev.h>
#include <assert.h>
#include <debug.h>
#include <string.h>

//设备数组
static device_t devices[NR_DEVICES];

static device_t* get_free_device() {
    for (size_t i = 1;i < NR_DEVICES;i++) {
        if (devices[i].type == DEV_NULL) {
            return &devices[i];
        }
    }
    return NULL;
}

device_t* device_get(dev_t dev) {
    return &devices[dev];
}

device_t* device_find(int subtype, size_t index) {
    for (size_t i = 1;i < NR_DEVICES;i++) {
        if (devices[i].sub_type != subtype) continue;
        if (--index == 0) return &devices[i];
    }
    return NULL;
}

dev_t device_install(const char* name, 
    dev_t parent, int type, int sub_type, void* ptr, 
    void* ioctl, void* read, void* write) {
    device_t* device = get_free_device();

    //为空
    if (!device) {
        LOGK("No free device");
        return -1;
    }
    memcpy(device->name, name, strlen(name));
    device->parent = parent;
    device->type = type;
    device->sub_type = sub_type;
    device->ptr = ptr;
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;

    return device->dev;
}



int device_ioctl(dev_t dev, int cmd, void *args, int flag) {
    device_t *device = device_get(dev);
    if (!device) {
        return -1;
    }
    return device->ioctl(device->ptr, cmd, args, flag);
}

int device_read(dev_t dev, void *buf, size_t count, uint32_t index, int flag) {
    device_t *device = device_get(dev);
    if (!device) {
        return -1;
    }
    return device->read(device->ptr, buf, count, index, flag);
}

int device_write(dev_t dev, void *buf, size_t count, uint32_t index, int flag) {
    device_t *device = device_get(dev);
    if (!device) {
        return -1;
    }
    return device->write(device->ptr, buf, count, index, flag);
}


int blk_device_request(dev_t dev, void *buf, size_t count, uint32_t index, int flag, uint32_t type);


void device_init() {
    for (size_t i = 0;i < NR_DEVICES;i++) {
        devices[i].dev = i;
        devices[i].type = DEV_NULL;
        devices[i].parent = 0;
        devices[i].ptr = NULL;
        devices[i].current_req = NULL;
    }
    //几个空的父设备
    memcpy(devices[HARDDISK_PARENT].name, "Hard Disk CTL", 14);
    devices[HARDDISK_PARENT].type = DEV_BLOCK;
}