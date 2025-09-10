#ifndef OS_BITMAP_H
#define OS_BITMAP_H

#include <stdint.h>

typedef struct bitmap_t {
    uint32_t bytes_len;       //位图大小
    uint32_t offset;    //位图起始偏移位置
    uint8_t *bits;      //位图指针
} bitmap_t;

//初始化
void bitmap_init(bitmap_t *bmap,uint8_t *bits, uint32_t bytes_len, uint32_t start);
//检查index是否是1
bool bitmap_test(bitmap_t *bmap, uint32_t index);
//扫描连续cnt个0
int bitmap_scan(bitmap_t *bmap, uint32_t cnt); 
void bitmap_set(bitmap_t *bmap, uint32_t index, bool value);

#endif