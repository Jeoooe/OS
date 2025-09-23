#include <mutex.h>
#include <interrupt.h>
#include <assert.h>

/* 申请和释放锁必须是原子操作 */
//申请锁
void lock_acquire(lock_t* lock) {
    bool state = interrupt_disable();
    task_block_t* task = running_task();

    //申请过还未释放
    if (lock->holder == task) {
        lock->repeat_nr++;
        return;
    }

    //信号量为0
    while (lock->signal == 0) {
        list_pushback(&lock->waiting_list, &task->node);
        task_block(TASK_BLOCKED);
    }

    //信号量为1, 则可以申请到锁
    lock->signal --;
    lock->holder = task;
    lock->repeat_nr = 1;

    set_interrupt_state(state);
}

//释放锁
void lock_release(lock_t* lock) {
    bool state = interrupt_disable();
    task_block_t *task = running_task();
    assert(lock->holder == task);   //必须是自己申请的锁
    assert(lock->signal == 0);      //信号量为0

    if (lock->repeat_nr > 1) {
        lock->repeat_nr --;
        return;
    }

    assert(lock->repeat_nr == 1);

    lock->repeat_nr = 0;
    lock->holder = NULL;
    lock->signal ++;

    //有等待锁的任务
    if (!list_empty(&lock->waiting_list)) {
        list_node_t *next_node = list_pop(&lock->waiting_list);
        task_block_t* next = element_entry(task_block_t, node, next_node);
        task_unblock(next);
    }

    set_interrupt_state(state);
}


void lock_init(lock_t* lock) {
    lock->holder = NULL;
    lock->repeat_nr = 0;
    lock->signal = 1;
    list_init(&lock->waiting_list);
}


//信号量初始化
void sema_init(semaphore_t* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

void sema_down(semaphore_t* psema) {
    bool state = interrupt_disable();
    while (psema->value == 0) { //已被持有
        list_pushback(&psema->waiters, &running_task()->node);
        task_block(TASK_BLOCKED);
    }
    psema->value--;
    assert(psema->value == 0);
    set_interrupt_state(state);
}

void sema_up(semaphore_t* psema) {
    bool state = interrupt_disable();
    assert(psema->value == 0);
    if (!list_empty(&psema->waiters)) {
        task_block_t* task = element_entry(task_block_t, node, list_pop(&psema->waiters));
        task_unblock(task);
    }
    psema->value++;
    assert(psema->value == 1);
    set_interrupt_state(state);
}