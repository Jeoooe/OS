#include <fs/file.h>
#include <thread.h>
#include <ide.h>
#include <fs/fs.h>
#include <debug.h>
#include <assert.h>
#include <os.h>
#include <string.h>

extern partition_t* cur_part;   //from fs.c

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

int32_t file_create(dir_t* parent_dir, char* filename, uint8_t flag) {
    void *io_buf = sys_malloc(1024);    //2扇区
    if (io_buf == NULL) {
        LOGK("Malloc fail [file_create]\n");
        return -1;
    }

    uint8_t rollback_step = 0;  //操作失败时回滚

    //inode分配
    int32_t inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1) {
        LOGK("Allocate inode fail [file_create]");
        return -1;
    }

    inode_t *new_file_inode = (inode_t*)sys_malloc(sizeof(inode_t));
    if (new_file_inode == NULL) {
        LOGK("Malloc for inode fail [file_create]\n");
        rollback_step = 1;
        goto rollback;
    }
    inode_init(inode_no, new_file_inode);

    int fd_index = get_free_slot_in_global();
    if (fd_index == -1) {
        LOGK("Exceed max open files\n");
        rollback_step = 2;
        goto rollback;
    }
    file_table[fd_index].fd_inode = new_file_inode;
    file_table[fd_index].fd_pos = 0;
    file_table[fd_index].fd_flag = flag;
    file_table[fd_index].fd_inode->write_deny = false;

    dir_entry_t new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(dir_entry_t));

    create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);

    /* 同步数据到硬盘 */
    //先安装目录项到父目录
    if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
        LOGK("Sync dir_entry fail");
        rollback_step = 3;
        goto rollback;
    }
    memset(io_buf, 0, 1024);

    //父目录inode内容同步
    inode_sync(cur_part, parent_dir->inode, io_buf);
    memset(io_buf, 0, 1024);

    //新建inode同步
    inode_sync(cur_part, new_file_inode, io_buf);
    
    //inode_bitmap同步
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    //新建inode添加到open_inodes
    list_push(&cur_part->open_inodes, &new_file_inode->inode_node);
    new_file_inode->i_open_cnts = 1;
    
    sys_free(io_buf);
    return pcb_fd_install(fd_index);

    
rollback:
    /* 创建失败回滚 */
    switch (rollback_step)
    {
    case 3:
        memset(&file_table[fd_index], 0, sizeof(file_t));
    case 2:
        sys_free(new_file_inode);
    case 1:
        bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
        break;
    }
    sys_free(io_buf);
    return -1;
}