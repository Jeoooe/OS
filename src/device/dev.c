#include <device/dev.h>
#include <assert.h>
#include <debug.h>
#include <string.h>
#include <os.h>

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
        if (index-- == 0) return &devices[i];
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

static request_t* request_nextreq(device_t* device, request_t* req) {
    list_t *list = &device->request_list;

    if (device->direct == DIRECT_UP && req->req_node.next == &list->tail) {
        device->direct = DIRECT_DOWN;
    }
    else if (device->direct == DIRECT_DOWN && req->req_node.prev == &list->head) {
        device->direct = DIRECT_UP;
    }

    void *next = NULL;
    if (device->direct == DIRECT_UP) {
        next = req->req_node.next;
    }
    else {
        next = req->req_node.prev;
    }

    //没有元素了
    if (next == &list->head || next == &list->tail) {
        return NULL;
    }
    return element_entry(request_t, req_node, next);
}

static int do_request(request_t* req) {
    switch (req->type)
    {
    case REQ_READ:
        return device_read(req->dev, req->buf, req->count, req->index, req->flag);
        break;
    case REQ_WRITE:
        return device_write(req->dev, req->buf, req->count, req->index, req->flag);
        break;
    default:
        panic("Req type %d unknown");
        break;
    }
    return -1;  //不会运行到这里, 仅消除编译器警告
}

int blk_device_request(dev_t dev, void *buf, 
    size_t count, uint32_t index, int flag, uint32_t type
    ) {
    device_t* blk = device_get(dev);
    request_t* req = (request_t*)kmalloc(sizeof(request_t));

    if (!blk) { //不存在
        return -1;
    }

    req->buf = buf;
    req->count = count;
    req->dev = blk->dev;
    req->flag = flag;
    req->index = index;
    req->type = type;
    req->task = running_task();
    
    
    bool empty = list_empty(&blk->request_list);
    //插入列表并维护有序
    list_insert_sort(&blk->request_list, &req->req_node, element_node_offset(request_t, req_node, index));

    //前面有请求
    if (!empty) {
        task_block(TASK_BLOCKED);
    }

    int ret = do_request(req);

    request_t* next_req = request_nextreq(blk, req);

    list_remove(&req->req_node);
    kfree(req);

    if (next_req) {
        assert(next_req->task->magic == MAGIC);
        task_unblock(next_req->task);
    }

    return ret;
}


void device_init() {
    for (size_t i = 0;i < NR_DEVICES;i++) {
        devices[i].dev = i;
        devices[i].type = DEV_NULL;
        devices[i].parent = 0;
        devices[i].ptr = NULL;
        list_init(&devices[i].request_list);
    }
}