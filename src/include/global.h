#ifndef OS_GLOBAL_H
#define OS_GLOBAL_H

#include <stdint.h>

typedef struct descriptor_ptr {
    uint16_t limit;
    uint32_t base;
} _packed descriptor_ptr;

#endif