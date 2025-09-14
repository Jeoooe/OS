#include <ring.h>
#include <assert.h>
#include <interrupt.h>

static inline uint32_t next_pos(uint32_t value) {
    return (value + 1) & (BUF_SIZE - 1);
}

void io_ring_init(io_ring_t* ring) {
    lock_init(&ring->lock);
    ring->consumer = ring->producer = NULL;
    ring->tail = ring->head = 0;
}

static void ring_wait(task_block_t** waiter) {
    assert(*waiter == NULL && waiter != NULL);
    *waiter = running_task();
    task_block(TASK_BLOCKED);
}

static void ring_wakeup(task_block_t** waiter) {
    assert(*waiter != NULL);
    task_unblock(*waiter);
    *waiter = NULL;
}

bool ring_full(io_ring_t *ring) {
    return (next_pos(ring->head) == ring->tail);
}

bool ring_empty(io_ring_t* ring) {
    return ring->head == ring->tail;
}


char io_ring_getchar(io_ring_t* ring) {
    assert(!get_interrupt_state());
    while(ring_empty(ring)) {
        //保证只能有一个消费者
        lock_acquire(&ring->lock);
        ring_wait(&ring->consumer);
        lock_release(&ring->lock);
    }

    char byte = ring->buf[ring->tail];
    ring->tail = next_pos(ring->tail);

    if (ring->producer) {
        ring_wakeup(&ring->producer);
    }

    return byte;
}

void io_ring_putchar(io_ring_t* ring, char byte) {
    assert(!get_interrupt_state());
    while(ring_full(ring)) {
        //保证只能有一个生产者
        lock_acquire(&ring->lock);
        ring_wait(&ring->producer);
        lock_release(&ring->lock);
    }

    ring->buf[ring->head] = byte;
    ring->head = next_pos(ring->head);

    if (ring->consumer) {
        ring_wakeup(&ring->consumer);
    }
}