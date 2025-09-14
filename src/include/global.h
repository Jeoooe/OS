#ifndef OS_GLOBAL_H
#define OS_GLOBAL_H

#include <stdint.h>

#define GDT_BASE 

#define RPL0 0
#define RPL3 3

#define SELECTOR_K_CODE (1 << 3) | RPL0  
#define SELECTOR_K_DATA (2 << 3) | RPL0  
#define SELECTOR_K_TSS  (3 << 3) | RPL0         
#define SELECTOR_U_CODE (4 << 3) | RPL3
#define SELECTOR_U_DATA (5 << 3) | RPL3


#define GDT_ATTR_HIGH           0b11000000
#define GDT_CODE_ATTR_LOW_DPL3  0b11111000
#define GDT_DAYA_ATTR_LOW_DPL3  0b11110010

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