#ifndef OS_CONSOLE_H
#define OS_CONSOLE_H

#include <stdint.h>

//控制台初始化
void console_init();

/// @brief 向控制台输出字符串
/// @param buf 字符串缓冲区
/// @param count 字符数
void console_write(const char* buf, size_t count);

#endif