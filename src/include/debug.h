#ifndef OS_DEBUG_H
#define OS_DEBUG_H

void debugk(char *file, int line, const char *fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx")

#ifndef NDEBUG
#define LOGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)
#else
#define LOGK(fmt, args...) 
#endif

#endif