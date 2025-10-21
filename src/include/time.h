#ifndef OS_TIME_H
#define OS_TIME_H

#include <stdint.h>

#define IRQ0_HZ 100

typedef struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
} tm;

void time_read(tm *time);
uint32_t mktime(tm* time);

extern uint32_t startup_time;  
extern uint32_t ticks; //内核中断以来总ticks

#define CURRENT_TIME (startup_time + ticks/IRQ0_HZ)


#endif