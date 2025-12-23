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
extern void tty_init();

static void test();
extern void uninstall_all_char_device();

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

    //部分设备
    uninstall_all_char_device();
    keyboard_init();
    tty_init();
    

    test();
}

//test部分
#include <fs/fs.h>
#include <fs/stat.h>
#include <fs/fcntl.h>
#include <syscall.h>
#include <thread.h>
#include <device/dev.h>

static void test() {

}