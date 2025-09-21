#ifndef OS_FS_H
#define OS_FS_H

#define MAX_FILES_PER_PART 4096 //最多创建文件数量
#define BITS_PER_SECTOR 4096    //扇区bit数
#define SECTOR_SIZE 512         //扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE  //块大小

typedef enum file_types {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
} file_types;

#endif