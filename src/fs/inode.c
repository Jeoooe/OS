#include <fs/inode.h>
#include <fs/fs.h>
#include <ide.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <interrupt.h>

typedef struct inode_position_t {
    bool two_sec;   //是否跨区
    uint32_t sec_lba;   //inode所在扇区
    uint32_t off_size;  //字节偏移量
} inode_position_t;

//获取inode位置
static void inode_locate(partition_t* part, uint32_t inode_no, inode_position_t* inode_pos) {
    assert(inode_no < MAX_FILES_PER_PART);

    const uint32_t inode_table_lba = part->super_block->inode_table_lba;
    const uint32_t inode_size = sizeof(inode_t);
    const uint32_t off_size = inode_no * inode_size;
    const uint32_t off_sec = off_size >> 9;    //扇区数, 即 / 512
    const uint32_t off_size_in_sec = off_size & 511;
    const uint32_t left_in_sec = 512 - off_size_in_sec;
    if (left_in_sec < inode_size) {
        //扇区剩余空间不足, inode跨区
        inode_pos->two_sec = true;
    }
    else {
        inode_pos->two_sec = false;
    }
    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

/* 将inode写入分区
 * io_buf是缓冲区,需要调用者申请好
 */
void inode_sync(partition_t* part, inode_t* inode, void* io_buf) {
    uint8_t inode_no = inode->i_no;
    inode_position_t inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    assert(inode_pos.sec_lba <= (part->start_lba + part->sector_cnt));

    //硬盘中inode不需要node和open_cnts
    inode_t pure_inode;
    memcpy(&pure_inode, inode, sizeof(inode_t));
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_node.prev = pure_inode.inode_node.next = NULL;
    char *inode_buf = (char*)io_buf;
    //若跨区,需要拼接
    const uint32_t write_sec_cnt = inode_pos.two_sec ? 2 : 1;
    ide_read(part->disk, inode_pos.sec_lba, inode_buf, write_sec_cnt);
    memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(inode_t));
    ide_write(part->disk, inode_pos.sec_lba, inode_buf, write_sec_cnt);
}

inode_t* inode_open(partition_t* part, uint32_t inode_no) {
    list_node_t* fnode = part->open_inodes.head.next;
    inode_t* inode_found;
    while (fnode != &part->open_inodes.tail) {
        inode_found = element_entry(inode_t, inode_node, fnode);
        if (inode_found->i_no == inode_no) {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        fnode = fnode->next;
    }

    //没有打开过
    inode_position_t inode_pos;
    inode_locate(part, inode_no, &inode_pos);

    //需要把inode置于内核空间
    task_block_t* task = running_task();
    uint32_t cur_pagedir = task->pd_addr;
    task->pd_addr = 0;  //偷偷改为内核线程
    inode_found = (inode_t*)sys_malloc(sizeof(inode_t));
    task->pd_addr = cur_pagedir;

    char *inode_buf;
    const uint32_t buf_secs = inode_pos.two_sec ? 2 : 1;
    inode_buf = (char*)sys_malloc(512 * buf_secs);
    ide_read(part->disk, inode_pos.sec_lba, inode_buf, buf_secs);
    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(inode_t));
    list_push(&part->open_inodes, &inode_found->inode_node);
    inode_found->i_open_cnts = 1;
    sys_free(inode_buf);
    return inode_found;
}


void inode_close(inode_t* inode) {
    bool state = interrupt_disable();
    inode->i_open_cnts--;
    if (inode->i_open_cnts == 0) {
        list_remove(&inode->inode_node);
        task_block_t* task = running_task();
        uint32_t cur_pagedir = task->pd_addr;
        task->pd_addr = 0;
        sys_free(inode);
        task->pd_addr = cur_pagedir;
    }
    set_interrupt_state(state);
}

//初始化新inode
void inode_init(uint32_t inode_no, inode_t* new_inode) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    uint8_t sec_idx = 0;
    while (sec_idx < 13) {
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}