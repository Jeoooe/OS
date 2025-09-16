#ifndef OS_TSS_H
#define OS_TSS_H

#include <thread.h>

//更新tss
void update_tss_esp(task_block_t* task);

#endif