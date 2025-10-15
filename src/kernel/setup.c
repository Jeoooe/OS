/* 
 * 系统进入用户态后的初始化
 */
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
#include <device/dev.h>
static void test() {
    device_t* part = device_find(DEV_IDE_PART, 0);

    //同步该分区的数据
    sync_dev(part->dev);
}