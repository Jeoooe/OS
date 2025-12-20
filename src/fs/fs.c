#include <fs/fs.h>
#include <fs/buffer.h>

int fs_ready = 0;   //文件系统是否准备好
file_t file_table[NR_FILE];