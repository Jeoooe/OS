#include <fs/fs.h>
#include <device/dev.h>
#include <ide.h>
#include <string.h>
#include <os.h>
#include <debug.h>
#include <assert.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//获取Inode数组起始块
#define INODE_START(sb) (2 + sb->zone_bitmap_blocks + sb->inode_bitmap_blocks)

#define LOG2_BLOCK_SECTOR 1
#define BLOCK_SIZE SECTOR_SIZE * (1 << LOG2_BLOCK_SECTOR)
#define NR_SUPER 8
#define ROOT_SUPER 0    //根目录超级块

//inode结构 
#define INODE_STRUCT_SIZE 32    //d_inode_t大小
#define INODE_PER_BLOCK BLOCK_SIZE / INODE_STRUCT_SIZE


super_block_t super_blocks[NR_SUPER];

//获取空闲的超级块
static super_block_t* get_free_super() {
    super_block_t* sb = super_blocks;
    for (;sb != &super_blocks[NR_SUPER];sb++) {
        if (sb->dev == 0) return sb;
    }
    return NULL;
}

//从超级块数组中获取已存在的超级快
static super_block_t* get_super(dev_t dev) {
    super_block_t* sb;
    for (sb = super_blocks;sb != &super_blocks[NR_SUPER]; sb++) {
        if (sb->dev == dev) 
            return sb;
    }
    return NULL;
}

//释放设备上的超级块
static void put_super(dev_t dev) {
    super_block_t* sb = get_super(dev);
    if (!sb) {
        panic("Cannot put a null dev");
    }  
    lock_acquire(&sb->lock);
    for (size_t i = 0;i < INODE_MAP_SIZE;i++) {
        if (sb->inode_map[i]) brelse(sb->inode_map[i]);
    }
    for (size_t i = 0;i < ZONE_MAP_SIZE;i++) {
        if (sb->zone_map[i]) brelse(sb->zone_map[i]);
    }
    //超级块写回磁盘
    buffer_t *buf = bread(dev, 1);
    d_super_block_t* dsb = (d_super_block_t*)buf->data;
    memcpy(dsb, sb, sizeof(d_super_block_t));
    buf->dirty = true;
    sb->dev = 0;
    brelse(buf);
    lock_release(&sb->lock);
}

//创建一个硬盘超级块
//同时会建立根目录
static void create_super(d_super_block_t* sb, dev_t dev) {
    const uint32_t sector_cnt = ((partition_t*)device_get(dev)->ptr)->sector_cnt; 
    sb->log_zone_size = LOG2_BLOCK_SECTOR;
    sb->inode_bitmap_blocks = INODE_MAP_SIZE;
    sb->zone_bitmap_blocks = ZONE_MAP_SIZE;
    //有一些奇特计算
    uint32_t free_blocks = sector_cnt / (1 << LOG2_BLOCK_SECTOR) - 2 - sb->zone_bitmap_blocks - sb->inode_bitmap_blocks;
    const uint32_t inode_cnt = INODE_MAP_SIZE * BLOCK_SIZE * 8;
    const uint32_t inode_blocks = DIV_ROUND_UP(free_blocks * INODE_PER_BLOCK, inode_cnt);

    if (free_blocks < inode_blocks) {
        //这什么鬼硬盘这么小
        panic("No free blocks in this disk");
    }

    free_blocks -= inode_blocks;
    sb->zones = free_blocks;
    sb->first_zone = 2 + sb->zone_bitmap_blocks + sb->inode_bitmap_blocks + inode_blocks;
    sb->inodes_count = 1;
    sb->max_size = 0;
    
    sb->magic = MAGIC;

    // /* 写入根目录 */
    // buffer_t *buf = bread(dev, 2);  //Inode位图
    // buf->data[1] = 0b11;
    // brelse(buf);    
    // buf = bread(dev, 2 + INODE_MAP_SIZE);
    // buf->data[1] = 0b11;
    // brelse(buf);
    // buf = bread(dev, INODE_START(sb));
    // d_inode_t* inode = buf->data;
    // inode++;
    // inode->zones[0] = 1;    //1号逻辑块
    // brelse(buf);
    // //根目录 目录项
    // //TODO
    /* 读入两个位图 */
}

//读取`dev`设备的超级块, 如果不存在则自动创建文件系统的超级块
//警告: 该函数会直接覆写该硬盘上原有的文件系统
static super_block_t* read_super(dev_t dev) {   
    if (dev == 0) {
        return NULL;
    }

    super_block_t* sb;
    sb = get_super(dev);    //已在数组中
    if (sb) {
        return sb;
    }
    //不在数组中
    sb = get_free_super();
    lock_acquire(&sb->lock);    //记得释放
    if (sb->dev) {  //已存在数组中 (二次确认, 以防可能的bug)
        goto roll_back2;
    }
    //不存在, 开始创建超级块
    buffer_t *buf = bread(dev, 1);  //读取超级块
    if (!buf) {
        //读取失败
        printk("Cannot read super_block from device %d\n", dev);
        goto roll_back1;
    }

    d_super_block_t* d_sb = (d_super_block_t*)buf->data;
    bool create = false;

    sb->dev = dev;
    //调试时候, 直接创建
    if (d_sb->magic == MAGIC && false) { //文件系统已经创建
        goto load_to_memory;
    }

    //该设备不存在超级块
    create_super(d_sb, dev);
    buf->dirty = true;
    create = true;
    
    //读入内存
load_to_memory:
    memcpy(sb, d_sb, sizeof(d_super_block_t));
    //这里没读出来直接死机 调试期间先这样吧
    uint32_t offset = 2;
    for (size_t i = 0;i < INODE_MAP_SIZE;i++) {
        sb->inode_map[i] = bread(dev, offset++);
        assert(sb->inode_map[i]);
    }
    for (size_t i = 0;i < ZONE_MAP_SIZE;i++) {
        sb->zone_map[i] = bread(dev, offset++);
        assert(sb->zone_map[i]);
    }

    //看是否需要创建根文件
    if (create) {
        sb->inode_map[0]->data[0] = 0b11;
        sb->zone_map[0]->data[0] = 0b11;    //第0块和第0个inode不要
        //读取inode数组
        buffer_t *tmp = bread(dev, offset);
        d_inode_t* di = (d_inode_t*)tmp->data;
        memset(di, 0, sizeof(d_inode_t));
        di->zones[0] = 1;
        tmp->dirty = true;
        brelse(tmp);
        //写入目录项
        tmp = bread(dev, sb->first_zone);
        dir_entry_t* de = (dir_entry_t*)tmp->data;
        de->filename[0] = '.';
        de->i_no = ROOT_INODE;
        de++;
        de->filename[0] = de->filename[1] = '.';
        de->i_no = ROOT_INODE;
        tmp->dirty = true;
        brelse(tmp);
    }

roll_back1:
    brelse(buf);
roll_back2:
    lock_release(&sb->lock);
    return sb;
}


//把设备挂载到inode上
static void mount_super() {

}

//挂载根目录
int mount_root(dev_t dev) {
    if (sizeof(d_inode_t) != INODE_STRUCT_SIZE) {
        panic("Inode structure error");
    }
    super_block_t* sb;
    //初始化超级块数组
    for (sb = super_blocks;sb < &super_blocks[NR_SUPER];sb++) {
        sb->dev = 0;
        sb->root_inode = sb->root_mount = NULL;
        memset(sb->inode_map, 0, sizeof(sb->inode_map));
        memset(sb->zone_map, 0, sizeof(sb->zone_map));
        lock_init(&sb->lock);
    }
    //读取超级块
    sb = read_super(dev);
    if (!sb) {
        panic("Unable to mount root");
    }
    return ROOT_INODE;
}

void root_setup() {
    mount_root(device_find(DEV_IDE_PART, 0)->dev);
}