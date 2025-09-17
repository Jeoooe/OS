#include <stdlib.h>
#include <syscall.h>

void *malloc(size_t size) {
    return (void*)_syscall1(SYS_MALLOC, size);
}
void free(void* ptr) {
    _syscall1(SYS_FREE, ptr);
}