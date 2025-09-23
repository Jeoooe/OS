#include <fs/fs.h>
#include <fs/dir.h>
#include <fs/inode.h>
#include <fs/super_block.h>
#include <fs/file.h>
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

extern dir_t root_dir;      //from dir.c

extern file_t file_table[]; //from file.c

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
    memset(buf, 0, buf_size);
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
    char default_part[8] = "hdb1";
    list_node_t* node = partition_list.head.next;
    for(; node != &partition_list.tail;node = node->next) {
        partition_t* p = element_entry(partition_t, part_node, node);
        if (strcmp(p->name, default_part) == 0) {
            mount_partition(p);
            break;
        }
    }
    //打开根目录
    open_root_dir(cur_part);
    uint32_t fd_index = 0;
    while (fd_index < MAX_FILE_OPEN) {
        file_table[fd_index++].fd_inode = NULL;
    }
}


//解析路径 name_store由调用者提供,返回下一级开头
static char* path_parse(char* path, char* name_store) {
    if (path[0] == '/') {
        while (*(++path) == '/');
    }
    while (*path != '/' && *path != '\0') {
        *name_store++ = *path++;
    }
    if (path[0] == '\0') {
        return NULL;
    }
    return path;
}

//路径深度
int32_t path_depth_cnt(char* pathname) {
    assert(pathname != NULL);
    char* p = pathname;
    char name[MAX_FILE_NAME_LEN];
    uint32_t depth = 0;

    p = path_parse(p, name);
    while (name[0]) {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN);
        if (p) {
            p = path_parse(p, name);
        }
    }
    return depth;
}


//搜索文件, 返回inode, 没找到返回-1
static int search_file(const char* path, struct path_search_record *searched_record) {
    if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
        searched_record->parent_dir = &root_dir;
        searched_record->file_type = FT_DIRECTORY;
        searched_record->searched_path[0] = 0;
        return 0;
    }

    uint32_t path_len = strlen(path);
    assert(path[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
    char* sub_path = (char*)path;
    dir_t* parent_dir = &root_dir;
    dir_entry_t dir_e;

    char name[MAX_FILE_NAME_LEN] = {0};

    searched_record->parent_dir = parent_dir;
    searched_record->file_type = FT_UNKNOWN;
    uint32_t parent_inode_no = 0;

    sub_path = path_parse(sub_path, name);
    while (name[0]) {
        strcat(searched_record->searched_path, "/");
        strcat(searched_record->searched_path, name);

        if (search_dir_entry(cur_part, parent_dir, name, &dir_e)) {
            memset(name, 0, MAX_FILE_NAME_LEN);
            if (sub_path) {
                sub_path = path_parse(sub_path, name);
            }

            if (dir_e.f_type == FT_DIRECTORY) { //打开了目录
                parent_inode_no = parent_dir->inode->i_no;
                dir_close(parent_dir);
                parent_dir = dir_open(cur_part, dir_e.i_no);
                searched_record->parent_dir = parent_dir;
                continue;
            }
            else if (dir_e.f_type == FT_REGULAR) {  //普通文件
                searched_record->file_type = FT_REGULAR;
                return dir_e.i_no;
            }
        }
        else {  //没找到目录项
            //保留parent_dir
            return -1;
        }
    }
    //完整路径, 且文件是目录
    dir_close(searched_record->parent_dir);
    searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
    searched_record->file_type = FT_DIRECTORY;
    return dir_e.i_no;
}



int32_t sys_open(const char* pathname, uint8_t flags) {
    if (pathname[strlen(pathname) - 1] == '/') {
        printk("Cant open a directory %s\n", pathname);
        return -1;
    }
    assert(flags <= 7);
    int32_t fd = -1;
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));

    //检查是否有目录不存在
    uint32_t pathname_depth = path_depth_cnt((char *)pathname);
    //检查文件是否存在
    int inode_no = search_file(pathname, &searched_record);
    bool found = inode_no != -1 ? true : false;

    if (searched_record.file_type == FT_DIRECTORY) {
        printk("Cant open a directory \n", pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }

    uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);

    //判断中间目录是否存在
    if (pathname_depth != path_searched_depth) {
        printk("cannot access %s: Not a directory, subpath %s is't exist\n", pathname, searched_record.searched_path);
        dir_close(searched_record.parent_dir);
        return -1;
    }

    //最后一个路径没找到,并且没有创建
    if (!found && !(flags & O_CREAT)) {
        printk("in path %s, file %s is't exist\n", searched_record.searched_path,
            (strrchr(searched_record.searched_path, '/') + 1));
        dir_close(searched_record.parent_dir);
        return -1;
    }
    else if (found && (flags & O_CREAT)) {  //要创建的已存在
        printk("%s has already exist\n", pathname);
        dir_close(searched_record.parent_dir);
        return -1;
    }

    switch (flags & O_CREAT)
    {
    case O_CREAT:   //创建文件
        printk("creating file\n");
        fd = file_create(searched_record.parent_dir,
            (strrchr(pathname, '/') + 1), flags);
        dir_close(searched_record.parent_dir);
        break;
    default:        //打开文件 
        fd = file_open(inode_no, flags);
        break;
    }
    return fd;
}


static uint32_t fd_local_to_global(uint32_t local_fd) {
    task_block_t *cur = running_task();
    int32_t global_fd = cur->fd_table[local_fd];
    return (uint32_t)global_fd;
}

int32_t sys_close(int32_t fd) {
    int32_t ret = -1;
    if (fd > 2) {
        uint32_t local_fd = fd_local_to_global(fd);
        ret = file_close(&file_table[local_fd]);
        running_task()->fd_table[fd] = -1;
    }
    return ret;
}