#include <fs/fs.h>
#include <fs/stat.h>
#include <device/dev.h>
#include <ide.h>
#include <string.h>
#include <os.h>
#include <debug.h>
#include <assert.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//获取Inode数组起始块
#define INODE_START(sb) (2 + sb->zone_bitmap_blocks + sb->inode_bitmap_blocks)

#define NR_SUPER 8

//inode结构 
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
super_block_t* get_super(dev_t dev) {
    super_block_t* sb;
    for (sb = super_blocks;sb != &super_blocks[NR_SUPER]; sb++) {
        if (sb->dev == dev) 
            return sb;
    }
    return NULL;
}

//释放设备上的超级块
void put_super(dev_t dev) {
    super_block_t* sb = get_super(dev);
    //一些不正确的行为
    if (!sb) {
        panic("Cannot put a null dev");
    }  
    //卸载根系统盘
    if (sb == &super_blocks[ROOT_SUPER]) {
        printk("Change root disk?");
        return;
    }
    //没处理安装到的inode
    if (sb->imount) {
        printk("Mounted disk changed");
        return;
    }
    //下面处理超级块的释放
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
    free_blocks = MIN(free_blocks, MAX_ZONE_COUNT); //有硬盘大小限制
    sb->zones = free_blocks;
    sb->first_zone = 2 + sb->zone_bitmap_blocks + sb->inode_bitmap_blocks + inode_blocks;
    sb->inodes_count = inode_cnt;   //这里是设置设备最多有多少inode
    sb->max_size = 0;
    
    sb->magic = MAGIC;

    //以下会设置两个位图0位为1, 占位不用
    //设置逻辑块位图第0个为1
    buffer_t* bh = bread(dev, 2 + INODE_MAP_SIZE);
    bh->data[0] = 1;
    bh->dirty = 1;
    brelse(bh);
    //设置inode位图第0个为1
    bh = bread(dev, 2);
    bh->data[0] = 1;
    bh->dirty = 1;
    brelse(bh); 
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


    sb->dev = dev;
    //调试时候, 直接创建
    if (d_sb->magic == MAGIC) { //文件系统已经创建
        goto load_to_memory;
    }

    //该设备不存在超级块
    buf->dirty = true;
    create_super(d_sb, dev);
    
    //把超级块读入内存
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

roll_back1:
    brelse(buf);
roll_back2:
    lock_release(&sb->lock);
    return sb;
}

//挂载超级块`dev`上的根节点
static inode_t *mount_root_inode(dev_t dev) {
    inode_t* root_inode = iget(dev, ROOT_INODE);
    if (root_inode->nlinks == 0) { 
        //根目录不存在
        iput(root_inode);
        root_inode = new_inode(dev);    //创建一个
        //创建一个逻辑块给他
        root_inode->zones[0] = new_block(dev);
        //设置根目录的属性
        root_inode->mode = S_IFDIR | 0755;
        root_inode->size = 2 * sizeof(dir_entry_t);
        root_inode->nlinks = 2;
        root_inode->dirty = 1;
        //然后写入两个目录项
        //到底为什么要用这么粗暴的方式来写根目录的两个目录项
        //但是我实在抄不到Linus的代码了
        buffer_t* bh = bread(dev, root_inode->zones[0]);    
        dir_entry_t *dentry = (dir_entry_t*)bh->data;
        dentry->i_no = ROOT_INODE;
        memcpy(dentry->filename, ".", 1);
        dentry++;
        dentry->i_no = ROOT_INODE;
        memcpy(dentry->filename, "..", 2);

        //写入目录项
        bh->dirty = 1;
        brelse(bh);
        //反正所有东西都同步到硬盘里面
        sync_dev(dev);
    }
    return root_inode;
}

//挂载根目录
void mount_root(dev_t dev) {
    if (sizeof(d_inode_t) != INODE_STRUCT_SIZE) {
        panic("Inode structure error");
    }
    super_block_t* sb;
    inode_t* root_inode;
    task_block_t* current = running_task();
    //初始化超级块数组
    for (sb = super_blocks;sb < &super_blocks[NR_SUPER];sb++) {
        sb->dev = 0;
        sb->isup = sb->imount = NULL;
        memset(sb->inode_map, 0, sizeof(sb->inode_map));
        memset(sb->zone_map, 0, sizeof(sb->zone_map));
        lock_init(&sb->lock);
    }
    //读取超级块
    sb = read_super(dev);
    if (!sb) {
        panic("Unable to mount root");
    }
    //读取根节点
    root_inode = mount_root_inode(dev);
    if (!root_inode) {
        panic("Unable to mount root inode");
    }
    //设置进程目录, 挂载超级块
    root_inode->count += 3;
    sb->isup = sb->imount = root_inode;
    current->pwd = root_inode;
    current->root = root_inode;
}

extern void fs_inode_init();
void root_setup() {
    //需要初始化inode_table
    fs_inode_init();
    mount_root(device_find(DEV_IDE_PART, 0)->dev);
}