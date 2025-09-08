#include <os.h>
#include <interrupt.h>
#include <stdint.h>
#include <debug.h>
#include <io.h>
#include <global.h>
#include <assert.h>

#define INTR_DESC_COUNT 0x21

#define PIC_M_CTRL 0x20 //主片控制端口
#define PIC_M_DATA 0x21 //主片数据端口
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

//全局变量
interrupt_desc_t idt[INTR_DESC_COUNT];
descriptor_ptr idt_ptr;

const char *intr_names[INTR_DESC_COUNT] = {
    "#DE Divide Error",
    "#DB Debug Exception",
    "NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "Coprocessor Segment Overrun",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection",
    "#PF Page Fault",
    "Reserved",
    "#MF x87 FPU Floating-Point Error",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD Floating-Point Exception"
};
//中断处理函数
intr_handler handler_table[INTR_DESC_COUNT];

void default_handler(uint8_t vector) {
    printk("[Default Handler] vector: %#x\n", vector);
    BMB;
}

extern intr_handler handler_entry_table[];
void create_idt_desc(interrupt_desc_t *desc, uint8_t attr, intr_handler handler) {
    desc->handler_low_16 = (uint32_t)handler & 0xffff;
    desc->handler_high_16 = ((uint32_t)handler >> 16) & 0xffff;
    desc->zeros = 0;
    desc->attributes = attr;
    desc->descriptor = 0b1000;
}

//初始化中断描述符表
static void idt_desc_init() {
    int i;
    for (i = 0;i < INTR_DESC_COUNT;i++) {
        create_idt_desc(&idt[i], 0b10001110, handler_entry_table[i]);
    }
}

//初始化中断控制器
static void pic_init() {
    //初始化主片
    outb(PIC_M_CTRL, 0x11);     //ICW1 边沿触发,级联
    outb(PIC_M_DATA, 0x20);     //ICW2 起始中断向量号为0x20(32)

    outb(PIC_M_DATA, 0x04);     //ICW3 IR2接从片
    outb(PIC_M_DATA, 0x01);     //ICW4 8086模式,正常EOI

    //初始化从片
    outb(PIC_S_CTRL, 0x11);     //ICW1 边沿触发,级联
    outb(PIC_S_DATA, 0x28);     //ICW2 起始中断向量号为0x28

    outb(PIC_S_DATA, 0x02);     //ICW3 接主片IR2
    outb(PIC_S_DATA, 0x01);     //ICW4 8086模式,正常EOI

    //时钟中断
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    LOGK("Pic init done");
}

static void handlers_init() {
    int i;
    for (i = 0;i < INTR_DESC_COUNT;i++) {
        handler_table[i] = default_handler;
    }
}

void interrupt_init() {
    LOGK("Interrupt Init...");
    idt_desc_init();
    handlers_init();
    pic_init();
    //加载IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)idt;
    asm volatile("lidt idt_ptr");
    LOGK("Idt_init done");
}