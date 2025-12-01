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

#include <fs/fs.h>
#include <fs/stat.h>
#include <syscall.h>
#include <thread.h>
static void test() {
    task_block_t* cur = running_task(); 
    // uint32_t fp = creat("/a.out", S_IFREG | 0777);
    // printk("Open a.out \n");
    // close(fp);
    int code = mkdir("/home", 0777);
    code = rmdir("/etc");
    printk("Create directory: /home");
    code = rmdir("/home");
    code = mkdir("/dev", 077);
    sync_dev(3);
}