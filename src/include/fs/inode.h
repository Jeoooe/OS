#ifndef OS_INODE_H
#define OS_INODE_H

#include <stdint.h>
#include <list.h>

typedef struct inode_t {
    uint32_t i_no;  //inode编号
    /* 当此inode是文件时，i_size是指文件大小, 
     * 若此inode是目录，i_size是指该目录下所有目录项大小之和
     * 单位为字节
     */ 
    uint32_t i_size;
    uint32_t i_open_cnts;   //被打开次数
    bool write_deny;    //写不可并行
    /* i_sectors[0-11]是直接块，i_sectors[12]用来存储一级间接块指针 */ 
    uint32_t i_sectors[13];
    list_node_t inode_node;
} inode_t;

/* 将inode写入分区
 * io_buf是缓冲区,需要调用者申请好
 */
void write_inode_to_part(partition_t* part, inode_t* inode, void* io_buf);

//打开一个inode
inode_t* inode_open(partition_t* part, uint32_t inode_no);

//关闭inode
void inode_close(inode_t* inode);

#endif