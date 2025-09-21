#ifndef OS_IDE_H
#define OS_IDE_H

#include <stdint.h>
#include <mutex.h>
#include <thread.h>
#include <fs/super_block.h>

typedef struct partition_t {
    uint32_t start_lba;     //起始扇区
    uint32_t sector_cnt;       //扇区数
    struct disk_t* disk;
    list_node_t part_node;
    char name[8];
    //文件系统
    super_block_t* super_block;
    bitmap_t block_bitmap;
    bitmap_t inode_bitmap;
    list_t open_inodes;
} partition_t;

typedef struct disk_t {
    char name[8];   
    struct ide_channel_t* ide;     //ide通道
    uint8_t dev_no;         //主0还是从1
    partition_t parts[4];
    partition_t logical_parts[8];
} disk_t;

typedef struct ide_channel_t {
    char name[8];
    uint16_t port_base;     //起始端口号
    uint8_t irq_no;         //中断号
    lock_t lock;
    bool expecting_intr;    //是否等待硬盘中断
    task_block_t* holder;   
    disk_t disks[2];
} ide_channel_t;

//写入硬盘sec_cnt个扇区
void ide_write(disk_t* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
//从硬盘读取sec_cnt个扇区
void ide_read(disk_t* hd, uint32_t lba, void* buf, uint32_t sec_cnt);

#endif