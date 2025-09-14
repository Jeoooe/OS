#include <timer.h>
#include <stdint.h>
#include <io.h>
#include <debug.h>
#include <thread.h>
#include <assert.h>
#include <os.h>
#include <interrupt.h>

#define INTR_TIME_VECTOR 0x20

#define IRQ0_HZ 100
#define INPUT_HZ 1193180
#define COUNTER0_VALUE INPUT_HZ / IRQ0_HZ
#define COUNTER0_PORT 0x40
#define COUNTER0_NUMBER 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL 0x40

uint32_t ticks; //内核中断以来总ticks

static void intr_timer_handler() {
    task_block_t* cur_task = running_task();
    assert(cur_task->magic == MAGIC);
    cur_task->elapsed_ticks ++;
    ticks++;
    if (cur_task->ticks == 0) {
        schedule();
    }   
    else {
        cur_task->ticks --;
    }
}

static void frequency_set() {
    outb(PIT_CONTROL, (uint8_t)(COUNTER0_NUMBER << 6 | READ_WRITE_LATCH << 4 | COUNTER_MODE << 1));
    outb(COUNTER0_PORT, (uint8_t)COUNTER0_VALUE);
    outb(COUNTER0_PORT, (uint8_t)(COUNTER0_VALUE>>8));
}

void timer_init() {
    LOGK("Timer init...");
    ticks = 0;
    frequency_set();
    register_handler(INTR_TIME_VECTOR, intr_timer_handler);
}