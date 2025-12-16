/* 
 * 系统进入用户态后的初始化
 */
#include <os.h>
#include <debug.h>
extern void ide_init();
extern void device_init();
extern void buffer_init();
extern void root_setup();
extern void keyboard_init();

static void test();

void sys_setup() {
    static int called = 0;
    if (called > 0) {
        return;
    }
    called ++;
    //设备有关
    device_init();
    buffer_init();
    ide_init();
    //文件系统
    root_setup();

    test();
    

    //这里应该就是停止了
    while (1);
}

//test部分
char buf[1024];

#include <fs/fs.h>
#include <fs/stat.h>
#include <fs/fcntl.h>
#include <syscall.h>
#include <thread.h>
static void test() {
    char ss[16];
    unsigned int fd = open("aaa", O_RDWR, 0777);
    read(fd, ss, 16);
    close(fd);
    sync_dev(3);
}