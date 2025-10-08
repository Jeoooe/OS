#ifndef OS_DEV_H
#define OS_DEV_H

#include <stdint.h>
#include <list.h>
#include <thread.h>

#define NR_DEVICES 64

//几个空的父设备
#define HARDDISK_PARENT 1

enum device_type{
    DEV_NULL,
    DEV_BLOCK,
    DEV_CHAR
};

enum sub_device_type {
    DEV_IDE_HARDDISK = 1,   //硬盘
    DEV_IDE_PART,           //硬盘分区
};

enum blk_device_direct {
    DIRECT_UP,
    DIRECT_DOWN
};

//请求项类型
#define REQ_READ 0
#define REQ_WRITE 1

typedef struct device_t {
    char name[16];
    dev_t dev;          //设备号
    dev_t parent;       //父设备号
    int type;           //设备类型
    int sub_type;       //子设备类型
    void *ptr;          //设备指针
    list_t request_list;    //请求列表
    int direct;         //寻道方向

    //设备对应的操作函数

    //dev传入设备指针
    int (*ioctl)(void* dev, int cmd, void *args, int flag);
    int (*read)(void* dev, void *buf, size_t count, uint32_t index, int flag);
    int (*write)(void* dev, void *buf, size_t count, uint32_t index, int flag);
} device_t;

typedef struct request_t {
    int dev;
    int type;
    uint32_t index;
    uint32_t count;
    int flag;
    char* buf;                  //数据缓冲区
    task_block_t *task;         //请求的任务
    list_node_t req_node;       //请求节点
} request_t;

//安装设备
dev_t device_install(const char* name, dev_t parent, int type, int sub_type, void* ptr, void* ioctl, void* read, void* write);

//根据设备号获取设备
device_t* device_get(dev_t dev);

//根据子设备类型寻找设备
device_t* device_find(int subtype, size_t index);

int device_ioctl(dev_t dev, int cmd, void *args, int flag);

int device_read(dev_t dev, void *buf, size_t count, uint32_t index, int flag);

int device_write(dev_t dev, void *buf, size_t count, uint32_t index, int flag);

//块设备请求, 失败返回-1
//`index`是相对于起始扇区的索引
int blk_device_request(dev_t dev, void *buf, size_t count, uint32_t index, int flag, uint32_t type);

#endif