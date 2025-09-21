#ifndef OS_DIR_H
#define OS_DIR_H

#include <stdint.h>
#include <list.h>
#include <fs/inode.h>
#include <fs/fs.h>

#define MAX_FILE_NAME_LEN 16

//目录结构
typedef struct dir_t {
    inode_t* inode;
    uint32_t dir_pos;       //目录内的偏移
    uint8_t dir_buf[512];
} dir_t;

typedef struct dir_entry_t {
    char filename[MAX_FILE_NAME_LEN];   //文件或者目录名字
    uint32_t i_no;                      //对应inode编号
    file_types f_type;
} dir_entry_t;


#endif