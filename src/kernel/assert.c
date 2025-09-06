#include <os.h>
#include <assert.h>

[[noreturn]] void assertion_failure(char* exp, char* file, char* base, int line) {
    printk("\n--> assert(%s) fail\n"
    "--> file: %s\n"
    "--> base_file %s\n"
    "--> line %d\n",
    exp, file, base, line);
    printk("assertion_failure");
    while(1)
        ;
}

[[noreturn]] void panic(const char *s) {
    printk("PANIC: %s\n", s);
    while (1) 
        ;
}