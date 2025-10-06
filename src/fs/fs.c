#include <fs/fs.h>
#include <device/dev.h>
#include <ide.h>
#include <debug.h>
#include <assert.h>
#include <os.h>
#include <string.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

#define ROOT_DIR_I_NO 1

extern ide_channel_t channels[];

dev_t root_system_dev;  //根目录所在分区的设备号

partition_t* cur_part;  //目前系统根目录所在分区

static void select_part(partition_t* part) {
    assert(part != NULL);
    cur_part = part;
}

//直接重置文件系统
static void filesystem_init(partition_t* part) {
    part->super_block = (super_block_t*)sys_malloc(sizeof(super_block_t));
    super_block_t* sp = part->super_block;
    ide_read(part->disk, sp, 1, part->start_lba + 1, 0);

    if (sp->magic == MAGIC) {
        //已存在超级块
        return;
    }
    //没有超级块, 则开始创建
    sp->sec_cnt = part->sector_cnt;
    sp->inode_cnt = MAX_INODE_COUNT;
    sp->part_lba_base = part->start_lba;
    const uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_INODE_COUNT / 8, SECTOR_SIZE);
    const uint32_t inode_table_sects = DIV_ROUND_UP((MAX_INODE_COUNT * sizeof(inode_t)), SECTOR_SIZE);
    uint32_t free_sects = sp->sec_cnt - 2 - inode_bitmap_sects - inode_table_sects;
    sp->block_bitmap_sects = DIV_ROUND_UP(free_sects, SECTOR_SIZE * 8 + 1);
    //计算到数据块起始地址
    sp->block_bitmap_lba = sp->part_lba_base + 2;
    sp->inode_bitmap_lba = sp->block_bitmap_lba + sp->block_bitmap_sects;
    sp->inode_table_lba = sp->inode_bitmap_lba + sp->inode_bitmap_sects;
    sp->data_start_lba = sp->inode_table_lba + sp->inode_table_sects;
    free_sects = sp->sec_cnt - sp->data_start_lba;

    sp->root_inode_no = ROOT_DIR_I_NO;
    sp->dir_entry_size = sizeof(dir_entry_t);
    //写入超级块
    ide_write(part->disk, sp, 1, part->start_lba + 1, 0);


    //处理各种位图
    uint32_t buf_size = MAX(sp->block_bitmap_sects, sp->inode_bitmap_sects);
    buf_size = MAX(buf_size, sp->inode_table_sects) * SECTOR_SIZE;
    uint8_t* buf = (uint8_t*)sys_malloc(buf_size);

    //块位图
    const uint32_t block_bmap_bytes = free_sects >> 3;
    uint32_t block_bmap_bits = free_sects & 7;
    memset(buf, 0, sp->block_bitmap_sects * SECTOR_SIZE);
    uint32_t i = block_bmap_bytes;
    while (block_bmap_bits > 0) {
        block_bmap_bits--;
        buf[i] |= 1 << block_bmap_bits;
    }
    buf[i] = ~buf[i];
    i++;
    for(;i < sp->block_bitmap_sects * SECTOR_SIZE;i++) {
        buf[i] = 0xff;
    }
    buf[0] = 1; //根目录占用第一个数据块
    ide_write(part->disk, buf, sp->block_bitmap_sects, sp->block_bitmap_lba, 0);

    //inode位图
    memset(buf, 0, sp->inode_bitmap_sects * SECTOR_SIZE);
    buf[0] = 0b11;  //0是保留不用, 1是根目录
    ide_write(part->disk, buf, sp->inode_bitmap_sects,  sp->inode_bitmap_lba, 0);

    //inode数组
    memset(buf, 0, sp->inode_table_sects * SECTOR_SIZE);
    inode_t* inode = (inode_t*)(&buf[ROOT_DIR_I_NO]);   //根目录
    inode->i_no = ROOT_DIR_I_NO;
    inode->i_size = 2 * sizeof(dir_entry_t);
    inode->i_type = FT_DIRECTORY;
    inode->i_zone[0] = sp->data_start_lba;
    ide_write(part->disk, buf, sp->inode_table_sects, sp->inode_table_lba, 0);

    //写入两个目录项 . 和 ..
    memset(buf, 0, sp->dir_entry_size * 2);
    // .
    dir_entry_t* entry = (dir_entry_t*)buf;
    entry->filename[0] = '.';                       
    entry->i_no = ROOT_DIR_I_NO;
    // ..
    entry++;
    entry->filename[0] = entry->filename[1] = '.';  
    entry->i_no = ROOT_DIR_I_NO;
    ide_write(part->disk, buf, 1, sp->data_start_lba, 0);

    sys_free(buf);
}


void fs_init() {
    //寻找第一个分区设备作为文件系统
    root_system_dev = device_find(DEV_IDE_PART, 1);
    select_part(&channels[0].disks[1].parts[0]);
    filesystem_init(cur_part);
}