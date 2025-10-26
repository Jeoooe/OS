#ifndef OS_MUTEX_H
#define OS_MUTEX_H

#include <stdint.h>
// #include <thread.h>
#include <list.h>

typedef struct lock_t {
    list_t waiting_list;    //等待任务的队列
    struct task_block_t* holder;   //持有者
    uint8_t signal;         //信号量
    uint8_t repeat_nr;      //重复申请锁的数量
} lock_t;

typedef struct semaphore_t {
    uint8_t value;
    list_t waiters;
} semaphore_t;

//初始化锁
void lock_init(lock_t* lock);

//申请锁
void lock_acquire(lock_t* lock);

//释放锁
void lock_release(lock_t* lock);

//信号量初始化
void sema_init(semaphore_t* psema, uint8_t value);

void sema_down(semaphore_t* psema);

void sema_up(semaphore_t* psema);


#endif