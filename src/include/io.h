#ifndef OS_IO_H
#define OS_IO_H

#include <stdint.h>

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern void outw(uint16_t port, uint16_t value);
extern uint16_t inw(uint16_t port);

#endif