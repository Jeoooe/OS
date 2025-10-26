#include <fs/fs.h>
#include <time.h>
#include <string.h>
#include <debug.h>
#include <assert.h>

#define NR_INODE 64 //内存inode数量

#define INODE_PER_BLOCK (BLOCK_SIZE / INODE_STRUCT_SIZE)

//文件最大数据块
#define MAX_BLOCKS_PER_FILE (7 + 512 + 512*512)

//内存inode表
static inode_t inode_table[NR_INODE];   

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

/* 文件数据块相关 */

static int _bmap(inode_t* inode, int block, int create) {
    assert(block >= 0);
    assert(block < MAX_BLOCKS_PER_FILE);

    buffer_t *bh;
    int i;

    //直接块
    if (block < 7) {
        if (create && !inode->zones[block]) {
            inode->zones[block] = new_block(inode->dev);
            if (inode->zones[block]) {
                inode->c_time = CURRENT_TIME;
                inode->dirty = 1;
            }
        }
        return inode->zones[block];
    }

    //一级块
    block -= 7;
    if (block < 512) {
        if (create && !inode->zones[7]) {
            inode->zones[7] = new_block(inode->dev);
            if (inode->zones[7]) {
                inode->c_time = CURRENT_TIME;
                inode->dirty = 1;
            }
        }
        if (!inode->zones[7]) return 0;
        //读取一级间接块
        bh = bread(inode->dev, inode->zones[7]);
        if (!bh) return 0;
        i = ((uint16_t*)(bh->data))[block];
        if (create && !i) {
            //创建
            i = new_block(inode->dev);
            if (i) {
                ((uint16_t*)bh->data)[block] = i;
                bh->dirty = 1;
            }
        }
        brelse(bh);
        return i;
    }

    //二级间接块
    block -= 512;
    if (create && !inode->zones[8]) {
        if ((inode->zones[8] = new_block(inode->dev))) {
            inode->c_time = CURRENT_TIME;
            inode->dirty = 1;
        }
    }
    if (!inode->zones[8]) 
        return 0;
    if (!(bh = bread(inode->dev, inode->zones[8]))) 
        return 0;
    i = ((uint16_t*)bh->data)[block>>9];
    if (create && !i) {
        if ((i = new_block(inode->dev))) {
            ((uint16_t*)(bh->data))[block>>9] = i;
            bh->dirty = 1;
        }
    }
    brelse(bh);
    if (!i) 
        return 0;
    if (!(bh=bread(inode->dev, i)))
        return 0;
    i = ((uint16_t*)bh->data)[block&511];
    if (create && !i) {
        if ((i=new_block(inode->dev))) {
            ((uint16_t*)(bh->data))[block & 511] = i;
            bh->dirty = 1;
        }
    }
    brelse(bh);
    return i;
}

//获取文件的数据块(文件的数据块表索引), 如果不存在则返回0
int get_block(inode_t *inode, int block) {
    return _bmap(inode, block, 0);
}

//获取文件的数据块(文件的数据块表索引), 如果不存在则创建
int create_block(inode_t* inode, int block) {
    return _bmap(inode, block, 1);
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
    d_inode_t* data = (d_inode_t*)bh->data;
    data[(inode->i_num - 1) % INODE_PER_BLOCK] = *(d_inode_t*)inode;
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