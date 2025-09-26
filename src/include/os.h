#ifndef OS_H
#define OS_H

#define MAGIC 799

#define DIV_ROUND_UP(x, n) (((x) + (n) - 1) / (n))

int printk(const char *fmt, ...);

#endif