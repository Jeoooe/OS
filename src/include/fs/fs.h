#ifndef OS_FS_H
#define OS_FS_H

#include <fs/buffer.h>
#include <mutex.h>

#define INODE_STRUCT_SIZE 32    //d_inode_t大小
//下面两个不要动了
#define LOG2_BLOCK_SECTOR 1     
#define BLOCK_SIZE 1024         //逻辑块大小

#define MAX_INODE_COUNT 8192    //文件系统中最大node数 好像也没用
#define MAX_FILE_NAME_LEN 30

#define INODE_MAP_SIZE 1
#define ZONE_MAP_SIZE 8     //逻辑块位图占据逻辑块数, 硬盘大小上限
#define MAX_ZONE_COUNT (BLOCK_SIZE * ZONE_MAP_SIZE * 8)

#define ROOT_INODE 1
#define ROOT_SUPER 0    //根目录超级块在超级块数组中的位置

enum file_type {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

/* 超级块 */
typedef struct super_block_t {
    uint16_t inodes_count;                  //文件系统的i节点数
    uint16_t zones;                         //逻辑块个数
    uint16_t inode_bitmap_blocks;           //inode位图块数
    uint16_t zone_bitmap_blocks;            //逻辑块位图
    uint16_t first_zone;                    //第一个数据块
    uint16_t log_zone_size;                 //log2(逻辑块大小/扇区大小)
    uint16_t max_size;                      //最大文件大小
    uint32_t magic;
    //以下仅内存中使用
    buffer_t* inode_map[INODE_MAP_SIZE];
    buffer_t* zone_map[ZONE_MAP_SIZE];      //从1开始计数
    dev_t dev;                              //对应设备号
    struct inode_t* isup;             //该文件系统的根目录
    struct inode_t* imount;             //该文件系统安装到的inode
    lock_t lock;                            
} super_block_t;

//硬盘中的超级块结构
typedef struct d_super_block_t {
    uint16_t inodes_count;                  //文件系统的i节点数
    uint16_t zones;                         //逻辑块个数
    uint16_t inode_bitmap_blocks;           //inode位图块数
    uint16_t zone_bitmap_blocks;            //逻辑块位图
    uint16_t first_zone;                    //第一个数据块
    uint16_t log_zone_size;                 //log2(逻辑块大小/扇区大小)
    uint16_t max_size;                      //最大文件大小
    uint32_t magic;                         //丰神
} d_super_block_t;

//内存中字段
typedef struct inode_t {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t mtime;
    uint8_t gid;
    uint8_t nlinks;     
    uint16_t zones[9];
    //以下是内存中独有的
    int i_num;      //inode数组下标
    dev_t dev;      //所属设备
    lock_t lock;
    uint16_t count; //引用数
    int dirty;      //是否已修改
    int i_pipe;     //是否是管道
    //时间
    uint32_t c_time; //修改时间
    uint32_t a_time; //访问时间
} inode_t;

//硬盘中的inode结构 应该是32字节
typedef struct d_inode_t {
    //刚好32字节
    uint16_t mode;          //文件属性
    uint16_t uid;           //宿主id
    uint32_t size;          //大小
    uint32_t mtime;         //修改时间
    uint8_t gid;            //文件组id
    uint8_t nlinks;         //链接数
    uint16_t zones[9];
} d_inode_t;

/* 目录项 应该刚好是32字节*/
typedef struct dir_entry_t {
    char filename[MAX_FILE_NAME_LEN];
    uint16_t i_no;  //指向的文件的inode
} dir_entry_t;

//文件
typedef struct file_t {
    uint16_t mode;          //文件操作模式
    uint16_t flags;         //文件打开和控制标志
    uint16_t count;         //对应文件引用计数
    inode_t* inode;         //对应inode
    uint32_t pos;           //读写位置
} file_t;




super_block_t* get_super(dev_t dev);

/* from inode.c */

//获取设备dev上nr号的节点
inode_t* iget(dev_t dev, int nr);
//释放节点, 如果修改过节点信息会写入
void iput(inode_t* inode);

//获取文件的数据块, 如果不存在则返回0
int get_block(inode_t *inode, int block);

//获取文件的数据块, 如果不存在则创建
int create_block(inode_t* inode, int block);


/* from bitmap.c */

//申请新inode
inode_t* new_inode(dev_t dev);
//申请新逻辑块, 返回逻辑块号
int new_block(dev_t dev);

void free_inode(inode_t* inode);
void free_block(dev_t dev, uint32_t block);



/* namei.c */

//根据文件名返回inode
//失败返回NULL
inode_t* namei(const char* pathname);

#endif