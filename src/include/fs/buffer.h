#ifndef OS_BUFFER_H
#define OS_BUFFER_H

#include <stdint.h>
#include <mutex.h>
#include <list.h>
#include <thread.h>

typedef struct buffer_t {
    char* data;
    uint32_t block;
    uint32_t dev;       //dev=0是空闲块
    uint32_t count;     //使用用户数
    int dirty;          //是否被修改
    int valid;          //是否有效

    lock_t lock;        //缓冲区锁
    // task_block_t *wait;     //等待的任务 (等待缓存有效)
    list_node_t free_node;  //空闲列表节点
    list_node_t hash_node;  //哈希列表的节点
} buffer_t;


buffer_t* get_blk(dev_t dev, uint32_t block);

//读取块设备并读入缓冲
//读取失败返回`NULL`
buffer_t *bread(dev_t dev, uint32_t block);

int bwrite(buffer_t* buf);

int brelse(buffer_t* buf);


#endif