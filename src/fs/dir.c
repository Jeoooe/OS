#include <fs/fs.h>
#include <fs/dir.h>
#include <ide.h>
#include <debug.h>
#include <string.h>
#include <assert.h>

dir_t root_dir;

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

void create_dir_entry(char* filename, uint32_t inode_no, file_types file_type, dir_entry_t* p_de) {
    const uint32_t namelen = strlen(filename);
    assert(namelen <= MAX_FILE_NAME_LEN);

    memcpy(p_de->filename, filename, namelen);
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}