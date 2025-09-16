#ifndef OS_USERPROG_H
#define OS_USERPROG_H

#define USER_STACK3_VADDR (0xc0000000 - 0x1000)

#define USER_VADDR_START 0x8048000

/* 构建用户进程上下文 */
void start_process(void* filename_);

//启动用户进程
void process_execute(void *filename, char* name);

#endif