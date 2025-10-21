#include <thread.h>

uint16_t sys_umask(uint16_t mask) {
    task_block_t *task = running_task();
    uint16_t old = task->umask;
    task->umask = mask & 0777;
    return old;
}