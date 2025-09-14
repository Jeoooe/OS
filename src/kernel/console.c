#include <stdint.h>
#include <stdio.h>
#include <io.h>
#include <debug.h>
#include <mutex.h>

#define MEM_BASE 0xb8000
#define MEM_END 0xc0000
#define CURSOR_MAX 2000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_HEIGHT * SCREEN_WIDTH)

#define CRT_ADDR_REG 0x3d4
#define CRT_DATA_REG 0x3d5

#define CURSOR_HIGH_REG 0xe
#define CURSOR_LOW_REG 0xf
#define SCREEN_START_ADDR_HIGH 0xc
#define SCREEN_START_ADDR_LOW 0xd

//特殊字符
#define BS 0x8      //退格
#define LF 0xa      //新行
#define CR 0xd      //换行

static uint16_t cursor_position = 0;
static uint16_t screen_position = 0;
static lock_t lock;

#define ADDR (MEM_BASE + (cursor_position + screen_position) * 2)

//设置光标位置
static inline void set_cursor() {
    outb(CRT_ADDR_REG, CURSOR_LOW_REG );    //低8位
    outb(CRT_DATA_REG, cursor_position & 0xFF);
    outb(CRT_ADDR_REG, CURSOR_HIGH_REG);    //高8位
    outb(CRT_DATA_REG, (cursor_position >> 8) & 0xFF);
}

//设置屏幕开始位置
static inline void set_screen() {
    outb(CRT_ADDR_REG, SCREEN_START_ADDR_LOW);    //高8位
    outb(CRT_DATA_REG, screen_position & 0xFF);
    outb(CRT_ADDR_REG, SCREEN_START_ADDR_HIGH );    //高8位
    outb(CRT_DATA_REG, (screen_position >> 8) & 0xff);
}

//清除屏幕
static inline void erase_screen() {
    uint16_t *ptr = (uint16_t *)MEM_BASE;
    for (;ptr != MEM_END;) {
        *ptr++ = 0;
    }
}

//上滚一行
static inline void scroll_up() {
    screen_position += 80;
    //超出显存
    if (screen_position >= MEM_END) {
        //复制一个屏幕到显存开头,然后设置屏幕为0
        //TODO
        screen_position = 0;
    }
    //将下一行清空
    uint16_t *ptr = (uint16_t*)(MEM_BASE + screen_position * 
        2 + (SCREEN_HEIGHT - 1) * SCREEN_WIDTH * 2);
    for (int i = 0;i < SCREEN_WIDTH;i++,ptr++) {
        *ptr = 0;
    }
    set_screen();
}

//换行
static inline void command_cr() {
    cursor_position -= cursor_position % 80;    //退到行首
    //判断是否超过最后一行
    if (cursor_position + 80 >= CURSOR_MAX) {
        scroll_up();
    }
    else {
        cursor_position += 80;
    }
}



//往屏幕输出一个字符
static void console_write_one(uint8_t ch) {
    uint16_t *ptr = NULL;
    switch (ch) {
    case BS:    //退格
        cursor_position--;
        ptr = (uint16_t *)ADDR;
        *ptr = 0;
        break;
    case CR:
    case LF:
        command_cr();
        break;
    default:    //正常字符
        ptr = (uint16_t *)ADDR;
        *ptr++ = (0x07 << 8) | (uint8_t)ch;
        cursor_position ++;
    }
    set_cursor();
}

void console_write(const char* buf, size_t count) {
    lock_acquire(&lock);
    uint8_t* ptr = (uint8_t *)buf;
    while (count-- > 0)
    {
        console_write_one(*ptr);
        ptr++;
    }
    lock_release(&lock);
}

void console_init() {
    cursor_position = 0;
    set_cursor();
    erase_screen();
    lock_init(&lock);
}