#ifndef OS_IO_H
#define OS_IO_H

#include <stdint.h>

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern void outw(uint16_t port, uint16_t value);
extern uint16_t inw(uint16_t port);
// extern void _insw(uint16_t port, void* addr, uint32_t word_cnt);
// extern void _outsw(uint16_t port, void* addr, uint32_t word_cnt);

static inline void _insw(uint16_t port, void* addr, uint32_t word_cnt) {
    asm volatile("cld; rep insw":"+D"(addr), "+c"(word_cnt):"d"(port):"memory");
}

static inline void _outsw(uint16_t port, void* addr, uint32_t word_cnt) {
    asm volatile("cld; rep outsw":"+S"(addr), "+c"(word_cnt):"d"(port):"memory");
}

#endif