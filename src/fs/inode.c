#include <fs/fs.h>
#include <string.h>
#include <debug.h>
#include <assert.h>

#define NR_INODE 64 //内存inode数量

#define INODE_PER_BLOCK BLOCK_SIZE / INODE_STRUCT_SIZE

//内存inode表
static inode_t inode_table[NR_INODE];   
static inode_t* root_inode;         //根目录inode指针

static void read_inode(inode_t* inode);
static void write_inode(inode_t* inode);


//同步全部的Inode到设备上
void sync_inode() {
    for (size_t i = 0;i < NR_INODE;i++) {
        inode_t* inode = &inode_table[i];
        lock_acquire(&inode->lock);
        if (inode->dirty && !inode->i_pipe) {
            write_inode(inode);
        }
        lock_release(&inode->lock);
    }
}

//给super.c加载根目录用的
void set_root_inode(inode_t* root) {
    if (root)
        root_inode = root;
}

//从内存inode表中获取空闲位置
inode_t* get_empty_inode() {
    for (inode_t *inode = inode_table;inode != &inode_table[NR_INODE];inode++) {
        if (inode->count != 0) continue;
        memset(inode, 0, sizeof(inode_t));
        lock_init(&inode->lock);
        inode->count = 1;
        return inode;
    }
    return NULL;
}

//获取inode指针
inode_t* iget(dev_t dev, int nr) {
    assert(dev > 0);
    //先遍历数组
    inode_t* inode = inode_table;
    while (inode != &inode_table[NR_INODE]) {
        if (inode->dev != dev || inode->i_num != nr) {
            inode++;
            continue;
        }
        lock_acquire(&inode->lock);
        //需要再判断一次
        if (inode->dev != dev || inode->i_num != nr) {
            inode = inode_table;
            continue;
        }

        //这里确认是找到了
        inode->count++;
        lock_release(&inode->lock);
        return inode;
    }
    //数组中没有
    //加载一个新的inode
    inode = get_empty_inode();
    if (!inode) return NULL;
    inode->dev = dev;
    inode->i_num = nr;
    read_inode(inode);
    return inode;
}

//释放inode
void iput(inode_t* inode) {
    if (!inode) return;

    lock_acquire(&inode->lock);
    
    //引用数
    if (inode->count > 1) {
        inode->count--;
        lock_release(&inode->lock);
        return;
    }

    //删除文件
    // if (inode->nlinks == 0) {
    //     //TODO
    // }

    //这里说明引用数为1, 释放后要写入磁盘

    //inode已经修改
    if (inode->dirty) {
        write_inode(inode);
    }

    inode->count--;

    lock_release(&inode->lock);
}



//从设备里面读取inode
static void read_inode(inode_t* inode) {
    assert(inode);
    int block;
    lock_acquire(&inode->lock);

    //先读取超级块
    super_block_t* sb = get_super(inode->dev);
    if (!sb) {
        panic("Read inode without dev");
    }

    //读取inode
    //得到逻辑块号
    block = 2 + sb->inode_bitmap_blocks + sb->zone_bitmap_blocks + \
        (inode->i_num - 1) / INODE_PER_BLOCK;
    buffer_t *bh = bread(inode->dev, block);
    if (!bh) {
        panic("Unable to read inode block");
    }
    *(d_inode_t*)inode = ((d_inode_t*)bh->data)[(inode->i_num - 1) % INODE_PER_BLOCK];
    
    brelse(bh);
    lock_release(&inode->lock);
}

//把inode节点信息写入磁盘的Inode数组对应的缓冲块中
static void write_inode(inode_t* inode) {
    //注意: 这里不可以申请锁
    int block;

    if (!inode->dirty || !inode->dev) 
        return;

    super_block_t* sb = get_super(inode->dev);
    if (!sb) {
        panic("Read inode without dev");
    }

    block = 2 + sb->inode_bitmap_blocks + sb->zone_bitmap_blocks + \
        (inode->i_num - 1) / INODE_PER_BLOCK;
    buffer_t *bh = bread(inode->dev, block);
    if (!bh) {
        panic("Unable to read inode block");
    }
    ((d_inode_t*)bh->data)[(inode->i_num - 1) % INODE_PER_BLOCK] = *(d_inode_t*)inode;
    bh->dirty = 1;
    inode->dirty = 0;
    brelse(bh);
    return;
}


//这里还是需要一个初始化函数来初始化inode_table里所有inode的锁
void fs_inode_init() {
    for (size_t i = 0;i < NR_INODE;i++) {
        lock_init(&inode_table[i].lock);
    }
}