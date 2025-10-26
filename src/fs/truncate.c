//文件截零  
#include <fs/fs.h>
#include <fs/stat.h>
#include <time.h>

static void free_ind(dev_t dev, uint32_t block) {
    if (!block) return;
    buffer_t* bh = bread(dev, block);
    if (bh) {
        uint16_t* p = (uint16_t*)bh->data;
        for (size_t i = 0;i < 512;i++, p++) {
            if (*p) {
                free_block(dev, *p);
            }
        }
        brelse(bh);
    }
    free_block(dev, block);
}

static void free_dind(dev_t dev, uint32_t block) {
    if (!block) return;
    buffer_t* bh = bread(dev, block);
    if (bh) {
        uint16_t* p = (uint16_t*)bh->data;
        for (size_t i = 0;i < 512;i++, p++) {
            if (*p) {
                free_ind(dev, *p);
            }
        }
        brelse(bh);
    }
    free_block(dev, block);
}

void truncate(inode_t* inode) {
    //判断inode有效性
    if (!inode) return;

    if (!(S_ISDIR(inode->mode) || S_ISREG(inode->mode))) {
        return;
    }

    //释放一级数据块
    for (int i = 0;i < 7;i++) {
        if (!inode->zones[i]) continue;
        free_block(inode->dev, inode->zones[i]);
        inode->zones[i] = 0;
    }
    free_ind(inode->dev, inode->zones[7]);
    free_dind(inode->dev, inode->zones[8]);
    inode->zones[7] = inode->zones[8] = 0;
    inode->size = 0;
    inode->dirty = 1;
    inode->mtime = inode->c_time = CURRENT_TIME;
}