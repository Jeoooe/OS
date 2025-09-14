#ifndef OS_GLOBAL_H
#define OS_GLOBAL_H

#include <stdint.h>

typedef struct descriptor_ptr {
    uint16_t limit;
    uint32_t base;
} _packed descriptor_ptr;

typedef struct gdt_desc_t {
    uint16_t limit_low_word;
    uint16_t base_low_word;
    uint8_t base_mid_byte;
    uint8_t attr_low_byte;
    uint8_t limit_high_attr_high;
    uint8_t base_high_byte;
} gdt_desc_t;

#endif