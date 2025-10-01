#ifndef OS_INTERRUPT_H
#define OS_INTERRUPT_H

#include <stdint.h>

typedef enum interrupt_type_e {
    INTERRUPT_TIMER = 0,
    INTERRUPT_KEYBOARD,
    INTERRUPT_SLAVE = 2,
    INTERRUPT_HARDDISK_MASTER = 14,
    INTERRUPT_HARDDISK_SLAVE = 15,
} interrupt_type_e;

typedef void* intr_handler;

typedef struct interrupt_desc_t {
    uint16_t handler_low_16;
    uint16_t descriptor;
    uint8_t zeros;
    uint8_t attributes;
    uint16_t handler_high_16;
} interrupt_desc_t;

//中断栈,发生中断时压入的结构
typedef struct interrupt_stack_t {
    uint32_t vector;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; 
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} interrupt_stack_t;

//设置中断开或关
void set_interrupt_state(bool state);
//获取中断是否开启
bool get_interrupt_state();
//关中断, 返回原先的状态
bool interrupt_disable();
//开中断, 返回原先的状态
bool interrupt_enable();

/// @brief 设置中断屏蔽
/// @param type 中断类型
/// @param mask 是否开启
void interrupt_mask(interrupt_type_e type, bool mask);


void register_handler(uint8_t vector, intr_handler handler);



#endif