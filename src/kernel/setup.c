/* 
 * 系统进入用户态后的初始化
 */
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
#include <device/dev.h>
#include <time.h>
static void test() {
    LOGK("Time: %d", CURRENT_TIME);
    tm time;
    time_read(&time);
    LOGK("Cur Time: %d/%d/%d-%d:%d:%d",
        time.tm_year,
        time.tm_mon,
        time.tm_mday,
        time.tm_hour,
        time.tm_min,
        time.tm_sec
    );
}