#include <fs/fs.h>
#include <fs/fcntl.h>
#include <errno.h>
#include <time.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))


int file_read(inode_t *inode, file_t *filp, char *buf, int count) {
    int left, chars, nr;
    buffer_t *bh;
    if (count <= 0) return 0;

    left = count;
    while (left) {
        if (nr = get_block(inode, (filp->pos) / BLOCK_SIZE)) {
            if (!(bh = bread(inode->dev, nr))) break;
        } else {
            bh = NULL;
        }
        //这里nr变成偏移值
        nr = filp->pos % BLOCK_SIZE;
        chars = MIN(BLOCK_SIZE - nr, left);
        filp->pos += chars;
        left -= chars;
        if (bh) {
            char *p = bh->data + nr;
            while (chars-- > 0) {
                *buf = *p;
                buf++; p++;
            }
            brelse(bh);
        } else {
            while (chars-- > 0) {
                *(buf++) = 0;
            }
        }
    }
    //设置访问时间
    inode->a_time = CURRENT_TIME;
    return (count - left) ? (count - left) : -ERROR;
}


int file_write(inode_t *inode, file_t *filp, char *buf, int count) {
    int pos;
    int block, c;
    buffer_t *bh;
    char *p;
    int i = 0;

    //是否追加模式
    if (filp->flags & O_APPEND) pos = inode->size;
    else pos = filp->pos;

    while (i < count) {
        if (!(block = create_block(inode, pos / BLOCK_SIZE))) {
            break;
        }
        if (!(bh = bread(inode->dev, block))) {
            break;
        }
        c = pos % BLOCK_SIZE;
        p = c + bh->data;
        bh->dirty = 1;
        c = BLOCK_SIZE - c;
        if (c > count - i) c = count - i;
        pos += c;
        if (pos > inode->size) {
            inode->size = pos;
            inode->dirty = 1;
        }
        i += c;
        while (c-- > 0) {
            *(p++) = *(buf++);
        }
        brelse(bh);
    }
    inode->mtime = CURRENT_TIME;    //修改时间
    if (!(filp->flags & O_APPEND)) {
        filp->pos = pos;
        inode->c_time = CURRENT_TIME;
    }
    return i ? i : -1;  //返回写入字节数
}