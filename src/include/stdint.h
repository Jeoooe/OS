#ifndef OS_STDINT_H
#define OS_STDINT_H

#define bool _Bool
#define true 1
#define false 0

#define NULL ((void *)0)
#define EOS '\0'
#define EOF -1

typedef char int8_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef unsigned int size_t;


#define _packed __attribute__((packed))


#endif