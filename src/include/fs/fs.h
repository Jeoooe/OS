#ifndef OS_FS_H
#define OS_FS_H

#define MAX_FILES_PER_PART 4096 //最多创建文件数量
#define BITS_PER_SECTOR 4096    //扇区bit数
#define BLOCK_SIZE SECTOR_SIZE  //块大小
#define MAX_PATH_LEN 512

#include <stdint.h>
#include <fs/dir.h>
#include <ide.h>

typedef enum file_types {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
} file_types;

enum open_flags {
    O_RDONLU = 0,
    O_WRONLY = 1,
    O_RDWR = 2,
    O_CREAT = 4
};

struct path_search_record {
    char searched_path[MAX_PATH_LEN];
    struct dir_t* parent_dir;
    file_types file_type;
};


//获取路径深度
int32_t path_depth_cnt(char* pathname);

/* 打开或创建文件, 返回文件描述符, 否则-1 */
int32_t sys_open(const char* pathname, uint8_t flags);

#endif