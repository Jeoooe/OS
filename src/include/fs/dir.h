#ifndef OS_DIR_H
#define OS_DIR_H

#include <stdint.h>
#include <list.h>
#include <fs/inode.h>
#include <fs/fs.h>
#include <ide.h>

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
    uint32_t f_type;
} dir_entry_t;

//打开分区根目录
void open_root_dir(partition_t* part);

//根据inode_no打开目录
dir_t* dir_open(partition_t* part, uint32_t inode_no);

//根据名字寻找目录项载入dir_e
bool search_dir_entry(partition_t* part, dir_t* pdir, const char* name, dir_entry_t* dir_e);

//关闭目录
void dir_close(dir_t* dir);

//创建目录项
void create_dir_entry(char* filename, uint32_t inode_no, uint32_t file_type, dir_entry_t* p_de);

//p_de写入父目录,需提供足够的io_buf, 512字节
bool sync_dir_entry(dir_t* parent_dir, dir_entry_t* p_de, void* io_buf);


#endif