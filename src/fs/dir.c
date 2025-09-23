#include <fs/fs.h>
#include <fs/dir.h>
#include <fs/file.h>
#include <ide.h>
#include <debug.h>
#include <string.h>
#include <assert.h>

dir_t root_dir;
extern partition_t* cur_part;   //from fs.c

void open_root_dir(partition_t* part) {
    root_dir.inode = inode_open(part, part->super_block->root_inode_no);
    root_dir.dir_pos = 0;
}

dir_t* dir_open(partition_t* part, uint32_t inode_no) {
    dir_t* pdir = (dir_t*)sys_malloc(sizeof(dir_t));
    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

bool search_dir_entry(partition_t* part, dir_t* pdir, const char* name, dir_entry_t* dir_e) {
    const uint32_t block_cnt = 140;   //12 + 128
    uint32_t *all_blocks = (uint32_t*)sys_malloc(140 * 4);
    if (all_blocks == NULL) {
        LOGK("Search_dir_entry: malloc fail");
        return false;
    }

    uint32_t i = 0;
    for (;i < 12;i++) {
        all_blocks[i] = pdir->inode->i_sectors[i];
    }
    i = 0;
    if (pdir->inode->i_sectors[12] != 0) {
        ide_read(part->disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }
    /*  allblocks存储该文件所有扇区地址 */
    uint8_t *buf = (uint8_t*)sys_malloc(SECTOR_SIZE);
    dir_entry_t* p_entry = (dir_entry_t*)buf;
    const uint32_t dir_entry_size = part->super_block->dir_entry_size;
    const uint32_t dir_entry_cnt = SECTOR_SIZE / dir_entry_size;

    for (;i < block_cnt;i++) {
        if (all_blocks[i] == 0) continue;
        ide_read(part->disk, all_blocks[i], buf, 1);

        uint32_t dir_entry_index = 0;
        while (dir_entry_index < dir_entry_cnt) {
            if (!strcmp(p_entry->filename, name)) {
                memcpy(dir_e, p_entry, dir_entry_size);
                sys_free(buf);
                sys_free(all_blocks);
                return true;
            }
            dir_entry_index++;
            p_entry++;
        }
        p_entry = (dir_entry_t*)buf;
        memset(buf, 0, SECTOR_SIZE);
    }
    sys_free(buf);
    sys_free(all_blocks);
    return false;
}

//关闭目录
void dir_close(dir_t* dir) {
    if (dir == &root_dir) return;
    inode_close(dir->inode);
    sys_free(dir);
}

void create_dir_entry(char* filename, uint32_t inode_no, uint32_t file_type, dir_entry_t* p_de) {
    const uint32_t namelen = strlen(filename);
    assert(namelen <= MAX_FILE_NAME_LEN);

    memcpy(p_de->filename, filename, namelen);
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}

//p_de写入父目录
bool sync_dir_entry(dir_t* parent_dir, dir_entry_t* p_de, void* io_buf) {
    inode_t* dir_inode = parent_dir->inode;
    const uint32_t dir_size = dir_inode->i_size;
    const uint32_t dir_entry_size = cur_part->super_block->dir_entry_size;

    assert(dir_size % dir_entry_size == 0);

    const uint32_t dir_entries_per_sec = (512 / dir_entry_size);

    int32_t block_lba = -1;

    uint8_t block_index = 0;
    uint32_t all_blocks[140] = {0};

    while (block_index < 12) {
        all_blocks[block_index] = dir_inode->i_sectors[block_index];
        block_index++;
    }

    dir_entry_t* dir_e = (dir_entry_t*)io_buf;

    int32_t block_bitmap_index = -1;
    block_index = 0;
    while (block_index < 140) { //140块
        block_bitmap_index = -1;
        if (all_blocks[block_index] == 0) {
            block_lba = block_bitmap_alloc(cur_part);
            if (block_lba == -1) {
                LOGK("alloc block fail for [sync_dir_entry]");
                return false;
            }

            /* 同步block_bitmap */
            block_bitmap_index = block_lba - cur_part->super_block->data_start_lba;
            assert(block_bitmap_index != -1);
            bitmap_sync(cur_part, block_bitmap_index, BLOCK_BITMAP);

            block_bitmap_index = -1;
            if (block_index < 12) { //直接块
                dir_inode->i_sectors[block_index] = all_blocks[block_index] = block_lba;
            }
            else if (block_index == 12) {
                //一级间接块表
                //第0个间接块
                dir_inode->i_sectors[12] = block_lba;
                block_lba = block_bitmap_alloc(cur_part);
                if (block_lba == -1) {
                    block_bitmap_index = dir_inode->i_sectors[12] - cur_part->super_block->data_start_lba;
                    bitmap_set(&cur_part->block_bitmap, block_bitmap_index, false);
                    dir_inode->i_sectors[12] = 0;
                    LOGK("alloc block bitmap fail for [sync_dir_entry]");
                    return false;
                }
                block_bitmap_index = block_lba - cur_part->super_block->data_start_lba;
                assert(block_bitmap_index != -1);
                bitmap_sync(cur_part, block_bitmap_index, BLOCK_BITMAP);
                all_blocks[12] = block_lba;
                ide_write(cur_part->disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
            }
            else {
                all_blocks[block_index] = block_lba;
                ide_write(cur_part->disk,
                    dir_inode->i_sectors[12], all_blocks + 12, 1);
            }
            //新目录项写入新分配间接块
            memset(io_buf, 0, 512);
            memcpy(io_buf, p_de, dir_entry_size);
            ide_write(cur_part->disk, all_blocks[block_index], io_buf, 1);
            dir_inode->i_size += dir_entry_size;
            return true;
        }
        //block_index块存在
        ide_read(cur_part->disk, all_blocks[block_index], io_buf, 1);
        uint8_t dir_entry_index = 0;
        while (dir_entry_index < dir_entries_per_sec) {
            if ((dir_e + dir_entry_index)->f_type == FT_UNKNOWN) {
                //初始化或删除文件后类型为UNKNOWN
                memcpy(dir_e + dir_entry_index, p_de, dir_entry_size);
                ide_write(cur_part->disk, all_blocks[block_index], io_buf, 1);
                dir_inode->i_size += dir_entry_size;
                return true;
            }
            dir_entry_index++;
        }
        block_index++;
    }
    LOGK("directory is full");
    return false;
}