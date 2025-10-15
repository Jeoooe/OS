/*
    bwrite和brelse函数的资源竞争部分是有隐患的, 如果出现问题可能需要修复
    BUG
*/
#include <fs/buffer.h>
#include <device/dev.h>
#include <ide.h>
#include <assert.h>
#include <os.h>

#define HASH_COUNT 301          // 哈希表项数
#define MAX_BUFFER_COUNT 301    //最多有几个缓冲块


#define BUFFER_SIZE 1024        //缓冲块大小
#define BUFFER_PAGES DIV_ROUND_UP(MAX_BUFFER_COUNT * BUFFER_SIZE, PAGE_SIZE) //缓冲块需要多少页
#define RW_SECTOR_COUNT BUFFER_SIZE / SECTOR_SIZE   //缓冲块读写的扇区数

#define HASH(dev, block) ((dev ^ block) % HASH_COUNT)

extern void sync_inode();

static buffer_t* start_buffer;  //buffer结构起始地址, 以数组形式存储buffer

static list_t wait_list;    //等待缓冲块的任务列表
static list_t free_list;    //空闲块列表
static list_t hash_table[HASH_COUNT];

static buffer_t* get_from_hashtable(dev_t dev, uint32_t block) {
    int index = HASH(dev, block);
    buffer_t *buf = NULL;
    list_node_t* node = hash_table[index].head.next;
    for (; node != &hash_table[index].tail; node = node->next) {
        buf = element_entry(buffer_t, hash_node, node);
        if (buf->block == block && buf->dev == dev) return buf;
    }
    return NULL;
}

//把缓冲块放入哈希表
static void push_hashtable(buffer_t* buf) {
    int index = HASH(buf->dev, buf->block);
    list_pushback(&hash_table[index], &buf->hash_node);
}

//缓冲块移出哈希表
static void remove_from_hashtable(buffer_t* buf) {
    list_remove(&buf->hash_node);
}

static buffer_t* get_free_buffer(dev_t dev, uint32_t block) {
    list_node_t* node = free_list.head.next;
    buffer_t* buf = NULL;
    while (true) {
        for (;node != &free_list.tail;node = node->next) {
            buf = element_entry(buffer_t, free_node, node);
            if (buf->count != 0) 
                continue;
            buf->dev = dev;
            buf->block = block;
            buf->count = 1;
            /*  这里不知道要不要判断是否已修改
                理论上, 释放掉的缓冲块应该已经同步了 */
            buf->valid = false;
            buf->dirty = false;
            //移出空闲列表
            list_remove(&buf->free_node);
            //放入哈希表中
            push_hashtable(buf);
            return buf;
        }
        //没有空闲
        //等待列表
        list_pushback(&wait_list, &running_task()->node);
        task_block(TASK_BLOCKED);
    }
}

buffer_t* get_blk(dev_t dev, uint32_t block) {
    buffer_t* buf = get_from_hashtable(dev, block);
    if (buf) {
        buf->count++;
        return buf;
    }
    //没有对应的缓存
    buf = get_free_buffer(dev, block);
    return buf;
}

buffer_t *bread(dev_t dev, uint32_t block) {
    buffer_t *buf;
    while (1) {
        buf = get_blk(dev, block);
        if (!buf) { //没有缓存块了
            return NULL;
        }
        
        lock_acquire(&buf->lock);   //锁住缓存

        if (buf->dev != dev || buf->block != block) {
            lock_release(&buf->lock);
            brelse(buf);
            continue;
        }

        //块已经在内存中

        if (buf->valid) {
            lock_release(&buf->lock);
            return buf;
        }
        else break;
    }
    
    int err = blk_device_request(dev, buf->data, RW_SECTOR_COUNT, block, 0, REQ_READ);

    if (err == -1) {    //读取失败
        lock_release(&buf->lock);
        brelse(buf);
        return NULL;
    }

    buf->valid = true;
    buf->dirty = false;
    lock_release(&buf->lock);
    return buf;
}

//缓冲写入块设备
int bwrite(buffer_t* buf) {
    if (!buf->dirty) {
        return 0;
    }

    buf->valid = false;
    lock_acquire(&buf->lock);
    
    //这里真是逆天bug
    //由于逻辑块和扇区大小不一致, 起始index也要乘上这个倍数
    uint32_t block = buf->block * RW_SECTOR_COUNT;
    int err = blk_device_request(buf->dev, buf->data, RW_SECTOR_COUNT, block, 0, REQ_WRITE);

    if (err != -1) {
        buf->dirty = false;
    }
    buf->valid = true;
    lock_release(&buf->lock);

    return err;
}

int brelse(buffer_t* buf) {
    if (!buf) {
        return -1;
    }
    buf->count--;
    assert(buf->count >= 0);
    if (buf->count > 0) {
        return 0;
    }
    //引用计数为0
    //需要检查是否被修改, 移出哈希表, 放入空闲列表, 
    if (buf->dirty) {
        //被修改过, 则同步到块设备
        bwrite(buf);
    }
    remove_from_hashtable(buf);
    list_pushback(&free_list, &buf->free_node);
    //唤醒正在等待缓冲块的任务
    if (!list_empty(&wait_list)) {
        task_block_t* task = element_entry(task_block_t, node, list_pop(&wait_list));
        task_unblock(task);
    }
    return 0;
}

int sync_dev(dev_t dev) {
    buffer_t* bh = start_buffer;
    for (size_t i = 0;i < MAX_BUFFER_COUNT;i++, bh++) {
        if (bh->dev != dev) continue;
        lock_acquire(&bh->lock);
        if (bh->dev == dev && bh->dirty) {
            lock_release(&bh->lock);
            bwrite(bh);
        }
    }
    sync_inode();
    bh = start_buffer;
    for (size_t i = 0;i < MAX_BUFFER_COUNT;i++, bh++) {
        if (bh->dev != dev) continue;
        lock_acquire(&bh->lock);
        if (bh->dev == dev && bh->dirty) {
            lock_release(&bh->lock);
            bwrite(bh);
        }
    }
    return 0;
}

void buffer_init() {
    list_init(&wait_list);
    list_init(&free_list);
    for (size_t i = 0;i < HASH_COUNT;i++) {
        list_init(&hash_table[i]);
    }
    
    /* 初始化缓冲块 */
    start_buffer = (buffer_t*)kmalloc(sizeof(buffer_t) * MAX_BUFFER_COUNT);
    buffer_t* buf = start_buffer;
    void *addr = get_kpages(BUFFER_PAGES);     //缓冲区起始位置
    for (size_t i = 0;i < MAX_BUFFER_COUNT; i++, addr += BUFFER_SIZE) {
        buf->block = buf->count = buf->dev = 0;
        buf->dirty = buf->valid = 0;
        buf->data = addr;
        lock_init(&buf->lock);
        list_push(&free_list, &buf->free_node);
        buf++;
    }
}