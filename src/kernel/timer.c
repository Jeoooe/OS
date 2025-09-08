#include <timer.h>
#include <stdint.h>
#include <io.h>
#include <debug.h>

#define IRQ0_HZ 100
#define INPUT_HZ 1193180
#define COUNTER0_VALUE INPUT_HZ / IRQ0_HZ
#define COUNTER0_PORT 0x40
#define COUNTER0_NUMBER 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL 0x40

static void set_frequency() {
    outb(PIT_CONTROL, (uint8_t)(COUNTER0_NUMBER << 6 | READ_WRITE_LATCH << 4 | COUNTER_MODE << 1));
    outb(COUNTER0_PORT, (uint8_t)COUNTER0_VALUE);
    outb(COUNTER0_PORT, (uint8_t)(COUNTER0_VALUE>>8));
}

void timer_init() {
    LOGK("Timer init...");
    set_frequency();
}