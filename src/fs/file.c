#include <fs/file.h>
#include <thread.h>
#include <ide.h>
#include <fs/fs.h>

file_t file_table[MAX_FILE_OPEN];

//文件表中获取空闲位
int32_t get_free_slot_in_global() {
    int32_t fd_index = 3;
    while (fd_index < MAX_FILE_OPEN) {
        if (file_table[fd_index].fd_inode == NULL) break;
        fd_index++;
    }
    if (fd_index == MAX_FILE_OPEN) {
        return -1;
    }
    return fd_index;
}


/* 安装文件描述符下标
 * 返回下标
 */
int32_t pcb_fd_install(int32_t global_fd_index) {
    task_block_t* task = running_task();
    uint8_t local_fd_idx = 3;
    while (local_fd_idx < MAX_FILE_OPEN) {
        if (task->fd_table[local_fd_idx] == -1) {
            task->fd_table[local_fd_idx] = global_fd_index;
            break;
        }
        local_fd_idx++;
    }
    if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {
        return -1;
    }
    return local_fd_idx;
}

//分配一个inode
int32_t inode_bitmap_alloc(partition_t* part) {
    int32_t bit_index = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_index == -1) {
        return -1;
    }
    bitmap_set(&part->inode_bitmap, bit_index, true);
    return bit_index;
}

//分配一个块, 返回扇区地址
int32_t block_bitmap_alloc(partition_t* part) {
    int32_t index = bitmap_scan(&part->block_bitmap, 1);
    if (index == -1) return -1;
    bitmap_set(&part->block_bitmap, index, true);
    return (part->super_block->data_start_lba + index);
}

//同步位图512字节到硬盘
void bitmap_sync(partition_t* part, uint32_t bit_index, enum bitmap_type btmp) {
    const uint32_t off_sec = bit_index / 4096;
    const uint32_t off_size = off_sec * BLOCK_SIZE;
    uint32_t sec_lba;
    uint8_t* bitmap_off;

    switch (btmp) {
    case INODE_BITMAP:
        sec_lba = part->super_block->inode_bitmap_lba + off_sec;
        bitmap_off = part->inode_bitmap.bits + off_size;
        break;
    case BLOCK_BITMAP:
        sec_lba = part->super_block->block_bitmap_lba + off_sec;
        bitmap_off = part->block_bitmap.bits + off_size;
        break;
    }
    ide_write(part->disk, sec_lba, bitmap_off, 1);
}