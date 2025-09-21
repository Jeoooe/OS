#include <fs/fs.h>
#include <fs/dir.h>
#include <fs/inode.h>
#include <fs/super_block.h>
#include <ide.h>
#include <os.h>
#include <debug.h>
#include <string.h>
#include <assert.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

extern uint8_t channel_cnt; //from ide.c
extern ide_channel_t channels[2];
extern int32_t ext_lba_base;   //总扩展分区起始lba
extern uint8_t partition_no, logical_no;

extern list_t partition_list;

partition_t* cur_part;  //默认操作的分区

//挂载分区
static void mount_partition(partition_t* part) {
    cur_part = part;
    disk_t* hd = cur_part->disk;

    super_block_t *sp_buf = (super_block_t*)sys_malloc(SECTOR_SIZE);
    cur_part->super_block = (super_block_t*)sys_malloc(sizeof(super_block_t));
    assert(cur_part->super_block != NULL);
    memset(sp_buf, 0, SECTOR_SIZE);
    ide_read(hd, cur_part->start_lba + 1, sp_buf, 1);

    memcpy(cur_part->super_block, sp_buf, sizeof(super_block_t));
    bitmap_init(&cur_part->block_bitmap, 
        (uint8_t*)sys_malloc(sp_buf->block_bitmap_sects * SECTOR_SIZE),
        sp_buf->block_bitmap_sects * SECTOR_SIZE, 0
    );

    assert(cur_part->block_bitmap.bits != NULL);
    //读入块位图
    ide_read(hd, sp_buf->block_bitmap_lba, 
        cur_part->block_bitmap.bits, sp_buf->block_bitmap_sects);
    
    //inode位图
    bitmap_init(&cur_part->inode_bitmap,
        (uint8_t*)sys_malloc(sp_buf->inode_bitmap_sects * SECTOR_SIZE),
        sp_buf->inode_bitmap_sects * SECTOR_SIZE, 0
    );
    assert(cur_part->inode_bitmap.bits != NULL);
    ide_read(hd, sp_buf->inode_bitmap_lba, cur_part->inode_bitmap.bits, sp_buf->inode_bitmap_sects);

    list_init(&cur_part->open_inodes);
    printk("mount %s done\n", part->name);
}

static void partition_format(partition_t* part) {
    const uint32_t boot_sector_sects = 1;
    const uint32_t super_block_sects = 1;
    const uint32_t inode_bitmap_sects = DIV_ROUND_UP(
        MAX_FILES_PER_PART, BITS_PER_SECTOR
    );  //inode位图扇区数
    const uint32_t inode_table_sects = DIV_ROUND_UP(
        (sizeof(inode_t) * MAX_FILES_PER_PART), SECTOR_SIZE
    );  //inode数组占用扇区数
    const uint32_t used_sects = boot_sector_sects + super_block_sects +
        inode_bitmap_sects + inode_table_sects;
    const uint32_t free_sects = part->sector_cnt - used_sects;

    //处理块位图
    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    const uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

    //超级块初始化
    super_block_t sp;
    sp.magic = MAGIC;
    sp.sec_cnt = part->sector_cnt;
    sp.inode_cnt = MAX_FILES_PER_PART;
    sp.part_lba_base = part->start_lba;

    sp.block_bitmap_lba = sp.part_lba_base + 2;
    sp.block_bitmap_sects = block_bitmap_sects;

    sp.inode_bitmap_lba = sp.block_bitmap_lba + sp.block_bitmap_sects;
    sp.inode_bitmap_sects = inode_bitmap_sects;

    sp.inode_table_lba = sp.inode_bitmap_lba + sp.inode_bitmap_sects;
    sp.inode_table_sects = inode_table_sects;

    sp.data_start_lba = sp.inode_table_lba + sp.inode_table_sects;
    sp.root_inode_no = 0;   //0inode为根目录
    sp.dir_entry_size = sizeof(dir_entry_t);

    //输出信息
    printk("%s info:\n", part->name);
    printk("magic:0x%x\n part_lba_base:%d\nall_sectors:%d\ninode_cnt:%d\n"
    "block_bitmap_lba:%d\n", sp.magic, sp.part_lba_base, sp.sec_cnt, sp.inode_cnt, sp.block_bitmap_lba);

    disk_t* hd = part->disk;

    /* 写入超级块 */
    ide_write(hd, part->start_lba + 1, &sp, 1);
    printk("SuperBlock: %d\n", part->start_lba + 1);

    /* 块位图 */
    uint32_t buf_size = MAX(sp.block_bitmap_sects, sp.inode_bitmap_sects);
    buf_size = MAX(buf_size, sp.inode_table_sects) * SECTOR_SIZE;
    uint8_t *buf = (uint8_t *)sys_malloc(buf_size);
    memset(buf, 0, buf_size);

    buf[0] |= 0b1;      //根目录
    const uint32_t block_bitmap_last_byte = block_bitmap_bit_len >> 3;
    const uint32_t block_bitmap_last_bit = block_bitmap_bit_len & 0b111;
    //最后一个扇区中剩余的空间
    const uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);  //全部置为1

    uint8_t bit_indx = 0;
    while (bit_indx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= ~(1 << bit_indx++);
    }
    ide_write(hd, sp.block_bitmap_lba, buf, sp.block_bitmap_sects);

    /* inode位图 */
    memset(buf, 0, sp.inode_bitmap_sects * 512);
    buf[0] |= 0b1;      //0inode 根目录
    // 4096个inode刚好一扇区, 无需处理剩余部分
    ide_write(hd, sp.inode_bitmap_lba, buf, sp.inode_bitmap_sects);

    /* inode数组初始化并写入table */
    memset(buf, 0, buf_size);
    inode_t* i = (inode_t*)buf; //根目录
    i->i_size = sp.dir_entry_size * 2;  //目录项 . and ..
    i->i_no = 0;
    i->i_sectors[0] = sp.data_start_lba;
    ide_write(hd, sp.inode_table_lba, buf, sp.inode_table_sects);

    /* 数据块写入根目录 */
    memset(buf, 0, buf_size);
    dir_entry_t* de = (dir_entry_t*)buf;

    //目录项: 当前目录 .
    memcpy(de->filename, ".", 1);
    de->i_no = 0;
    de->f_type = FT_DIRECTORY;
    de++;

    //目录项: 父目录 ..
    memcpy(de->filename, "..", 2);
    de->i_no = 0;               //依然根目录
    de->f_type = FT_DIRECTORY;

    ide_write(hd, sp.data_start_lba, buf, 1);

    printk("Root Directory LBA: %d\n", sp.data_start_lba);
    LOGK("%s Format Done", part->name);
    sys_free(buf);
}

//搜索文件系统,或者创建文件系统
void filesys_init() {
    uint8_t channel_no = 0, dev_no, part_index = 0;
    super_block_t* sp_buf = (super_block_t*)sys_malloc(SECTOR_SIZE);

    assert(sp_buf != NULL);
    printk("Searching filesystem...\n");
    while (channel_no < channel_cnt) {
        dev_no = 0;
        while (dev_no < 2) {
            if (dev_no == 0) {  //跳过裸盘,即只装了内核代码的
                dev_no++;
                continue;
            }
            disk_t* hd = &channels[channel_no].disks[dev_no];
            partition_t* part = hd->parts;
            while (part_index < 12) {
                //4主+8逻辑
                if (part_index == 4) {
                    part = hd->logical_parts;
                }
                if (part->sector_cnt != 0) {
                    //分区存在
                    memset(sp_buf, 0, SECTOR_SIZE);
                    //读出超级块
                    ide_read(hd, part->start_lba + 1, sp_buf, 1);
                    if (sp_buf->magic == MAGIC) {
                        printk("%s has filesystem\n", part->name);
                    }
                    else {
                        printk("%s has no filesystem\n", part->name);
                        partition_format(part);
                    }
                }
                part_index++;
                part++; //下一个分区
            }
            dev_no++;   //下一个磁盘
        }
        channel_no++;   //下一个通道
    }
    sys_free(sp_buf);

    //挂载分区
    list_node_t* node = partition_list.head.next;
    for(; node != &partition_list.tail;node = node->next) {
        partition_t* p = element_entry(partition_t, part_node, node);
        if (strcmp(p->name, "hdb2") == 0) {
            mount_partition(p);
            break;
        }
    }
}