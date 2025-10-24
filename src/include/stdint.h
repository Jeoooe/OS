#ifndef OS_STDINT_H
#define OS_STDINT_H

#define bool _Bool
#define true 1
#define false 0

#define NULL ((void *)0)
#define EOS '\0'
#define EOF -1

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef int size_t;

typedef int16_t pid_t;

typedef int dev_t;
typedef short mode_t;


#define _packed __attribute__((packed))


#endif