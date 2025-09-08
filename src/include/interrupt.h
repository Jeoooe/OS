#ifndef OS_INTERRUPT_H
#define OS_INTERRUPT_H

#include <stdint.h>

typedef void* intr_handler;

typedef struct interrupt_desc_t {
    uint16_t handler_low_16;
    uint16_t descriptor;
    uint8_t zeros;
    uint8_t attributes;
    uint16_t handler_high_16;
} interrupt_desc_t;

//设置中断开或关
void set_interrupt_state(bool state);
//获取中断是否开启
bool get_interrupt_state();

#endif