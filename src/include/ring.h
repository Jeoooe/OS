#ifndef OS_RING_H
#define OS_RING_H

#include <mutex.h>
#include <thread.h>

#define BUF_SIZE 64 //必须保证是2的幂次

//环形缓冲区
typedef struct io_ring_t {
    lock_t lock;                //线程锁,同时只能有一个消费者或一个生产者
    task_block_t *producer;
    task_block_t *consumer;
    char buf[BUF_SIZE];         //缓冲区
    uint32_t tail;
    uint32_t head;
} io_ring_t;


void io_ring_init(io_ring_t* ring);

bool ring_full(io_ring_t *ring);

bool ring_empty(io_ring_t* ring);

char io_ring_getchar(io_ring_t* ring);

void io_ring_putchar(io_ring_t* ring, char byte);

#endif