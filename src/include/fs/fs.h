#ifndef OS_FS_H
#define OS_FS_H

#include <ide.h>

#define MAX_INODE_COUNT 4096
#define BLOCK_SIZE SECTOR_SIZE
#define MAX_FILE_NAME_LEN 30

enum file_type {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

/* 超级块 */
typedef struct super_block_t {
    uint32_t magic;
    uint32_t sec_cnt;
    uint32_t inode_cnt;
    uint32_t part_lba_base;     //分区起始lba
    uint32_t block_bitmap_lba;  //块位图起始扇区
    uint32_t block_bitmap_sects;
    uint32_t inode_bitmap_lba;
    uint32_t inode_bitmap_sects;
    uint32_t inode_table_lba;
    uint32_t inode_table_sects;
    uint32_t data_start_lba;
    uint32_t root_inode_no;
    uint32_t dir_entry_size;
    uint8_t pad[512 - (__LINE__ - 8)*4];    //填补到一个扇区
} _packed super_block_t;

typedef struct inode_t {
    uint32_t i_type;
    uint32_t i_size;    //文件大小
    uint32_t i_no;
    //扇区索引
    uint32_t i_zone[9];    // 7 一次间接块, 8 二次间接块
} inode_t;

/* 目录项 应该刚好是32字节*/
typedef struct dir_entry_t {
    uint16_t i_no;  //指向的文件的inode
    char filename[MAX_FILE_NAME_LEN];
} dir_entry_t;

#endif