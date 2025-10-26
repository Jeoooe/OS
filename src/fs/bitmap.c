/*
    文件系统的位图使用缓冲区, 其位图不连续
    干脆特供一个
*/
#include <os.h>
#include <fs/fs.h>
#include <thread.h>
#include <time.h>
#include <string.h>
#include <assert.h>

extern inode_t* get_empty_inode();

//块操作
//这些汇编实在逆天

//清空addr开始的1024字节
#define clear_block(addr) \
asm("cld\n"\
    "rep stosl"\
    ::"a"(0), "c"(BLOCK_SIZE / 4), "D"((uint32_t)(addr)))

//位操作
#define set_bit(addr, offset) ({ \
    register int res;   \
    asm volatile("btsl %2, %3\nsetb %%al"\
    :"=a"(res):"0"(0), "r"(offset), "m"(*(addr)));  \
    res;})

#define clear_bit(addr, offset) ({ \
    register int res;   \
    asm volatile("btrl %2, %3\nsetnb %%al"\
    :"=a"(res):"0"(0), "r"(offset), "m"(*(addr)));  \
    res;})

//一个逻辑块中找到第一个0
#define find_first_zero(addr) ({  \
    register int __res; \
    asm("cld\n"     \
    "1:\tlodsl\n"\
    "notl %%eax\n"\
    "bsfl %%eax, %%edx\n"\
    "je 2f\n"\
    "addl %%edx, %%ecx\n"\
    "jmp 3f\n"\
    "2:\taddl $32, %%ecx\n"\
    "cmpl $8192, %%ecx\n"\
    "jl 1b\n"\
    "3:"\
    :"=c"(__res):"c"(0), "S"(addr));   \
    __res;})

//从设备里得到一个新的空Inode
inode_t* new_inode(dev_t dev) {
    super_block_t* sb = get_super(dev);
    inode_t* inode = get_empty_inode();

    if (!inode) 
        return NULL;
    if (!sb) {
        panic("No Super Block in this device");
    }

    int offset = 8192, i = 0;
    buffer_t *bh = NULL;
    for (;i < sb->inode_bitmap_blocks;i++) {
        bh = sb->inode_map[i];
        if (bh) {
            offset = find_first_zero(bh->data);
            if (offset < 8192) break;
        }
    }
    if (!bh || offset >= 8192 || offset + i*8192 > MAX_INODE_COUNT) {
        iput(inode);
        return NULL;
    }

    //已经得到了i节点号
    if (set_bit(bh->data, offset)) {
        panic("Bit already set");
    }

    task_block_t* current = running_task();
    bh->dirty = 1;
    inode->count = 1;
    inode->nlinks = 1;
    inode->dev = dev;
    inode->uid = current->euid;
    inode->gid = current->gid;
    inode->dirty = 1;
    inode->i_num = offset + i*8192;
    inode->mtime = inode->a_time = inode->c_time = CURRENT_TIME;
    return inode;
}  


int new_block(dev_t dev) {
    int i, j = 8192;
    buffer_t* bh;
    super_block_t* sb = get_super(dev);
    if (!sb) {
        panic("Nonexistant device");
    }
    
    for (i = 0;i < sb->zone_bitmap_blocks;i++) {
        bh = sb->zone_map[i];
        if (bh) {
            if ((j = find_first_zero(bh->data)) < 8192) {
                break;
            }
        }
    }
    //反正是没有块了
    if (!bh || i >= sb->zone_bitmap_blocks || j >= 8192) {
        return 0;
    }
    //找到了
    if (set_bit(bh->data, j)) {
        panic("new_block: bit already set");
    }
    bh->dirty = 1;  //设置位图的缓冲块已修改
    j += i * 8192 + sb->first_zone - 1;
    //也不能超出块总数
    if (j >= sb->zones) {
        return 0;
    }

    if (!(bh = get_blk(dev, j))) {
        panic("new_block: cannot get block");
    }
    if (bh->count != 1) {
        panic("new_block: count != 1");
    }
    clear_block(bh->data);
    bh->dirty = 1;
    bh->valid = 1;
    brelse(bh);
    return j;
}

void free_block(dev_t dev, uint32_t block) {
    super_block_t* sb;
    buffer_t* bh;

    if (!(sb = get_super(dev))) {
        panic("try to free block on nonexistent device");
    }
    if (block < sb->first_zone || block >= sb->zones) {
        panic("try to free block not in datazone");
    }
    //检查缓冲区中是否有该块数据, 如果有就释放掉
    bh = get_from_hashtable(dev, block);
    if (bh) {
        if (bh->count > 1) {
            brelse(bh);
            return;
        }
        bh->dirty = 0;
        bh->valid = 0;
        if (bh->count) brelse(bh);
    }
    //复位比特位
    block -= sb->first_zone - 1;
    if (clear_bit(sb->zone_map[block / 8192]->data, block & 8191)) {
        //已经是0
        printk("block (%d: 0x%x)\n", dev, block + sb->first_zone - 1);
        panic("free_bl0ock: bit already cleared");
    }
    sb->zone_map[block / 8192]->dirty = 1;
}

void free_inode(inode_t* inode) {
    //inode是否合理
    if (!inode) return;
    if (!inode->dev) {  //节点未使用
        memset(inode, 0, sizeof(inode_t));
        lock_init(&inode->lock);
        return;
    }
    //还有引用, 内核是有问题的
    if (inode->count > 1) {
        printk("try to free inode with count=%d\n", inode->count);
        panic("free_inode");
    }
    if (inode->nlinks) {    
        //还有文件目录引用该节点
        //文件还未删除
        panic("try to free inode with links");
    }

    //inode是否正确
    super_block_t* sb = get_super(inode->dev);
    if (!sb) {
        panic("try to free inode with nonexistent device");
    }
    if (inode->i_num < 1 || inode->i_num > sb->inodes_count) {
        panic("try to free inode 0 or nonexistent inode");
    }
    buffer_t* bh = sb->inode_map[inode->i_num >> 13];
    if (!bh) {
        panic("nonexistent imap in superblock");
    }

    if (clear_bit(bh->data, inode->i_num & 8191)) {
        printk("free_inode: bit already cleared\n");
    }
    bh->dirty = 1;
    memset(inode, 0, sizeof(inode_t));
    lock_init(&inode->lock);
}

