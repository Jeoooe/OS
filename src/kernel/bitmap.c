#include <bitmap.h>
#include <string.h>
#include <assert.h>
#include <debug.h>

#define MUL_8(num) ((num) << 3)
#define DIV_8(num) ((num) >> 3)
#define MOD_8(num) ((num) & 0b111)

//初始化
void bitmap_init(bitmap_t *bmap,uint8_t *bits, uint32_t bytes_len, uint32_t start) {
    bmap->bits= bits;
    bmap->bytes_len = bytes_len;
    bmap->offset = start;
    memset(bmap->bits, 0, bmap->bytes_len);
}

//检查index是否是1
bool bitmap_test(bitmap_t *bmap, uint32_t index) {
    assert(index < MUL_8(bmap->bytes_len));
    return (bmap->bits[DIV_8(index)] >> MOD_8(index)) & 1;
}

//扫描连续cnt个0
//返回-1是未找到
int bitmap_scan(bitmap_t *bmap, uint32_t cnt) {
    uint32_t nr = 0;
    const uint32_t max_size = MUL_8(bmap->bytes_len);
    uint32_t i = 0;
    //可以先跨过一些字节
    while (bmap->bits[i] == 0xff) i += 8;
    for (;i < max_size;i++) {
        while (!bitmap_test(bmap, i + nr)) {
            nr++;
            if (nr == cnt) return i;
            if (i + nr >= max_size) return -1;
        }
        i += nr;
        nr = 0;
    }
    return -1;
}

void bitmap_set(bitmap_t *bmap, uint32_t index, bool value) {
    assert(index < MUL_8(bmap->bytes_len));
    if (value) {
        bmap->bits[DIV_8(index)] |= (1 << MOD_8(index));
    }
    else {
        bmap->bits[DIV_8(index)] &= ~(1 << MOD_8(index));
    }
}