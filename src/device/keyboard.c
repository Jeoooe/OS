#include <stdint.h>
#include <interrupt.h>
#include <os.h>
#include <io.h>
#include <console.h>
#include <ring.h>

#define KEYBOARD_PORT 0x60
#define KEYBOARD_VECTOR 0x21

#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

#define INV 0

/* 全局变量 */
io_ring_t kb_buf;

//键盘扫描码
static char keymap[][2] = {
    /* 扫描码 {无shift组合, shift组合} */
    /* 0x00 */ {INV,      INV, },   // NULL
    /* 0x01 */ {0x1b,    0x1b,},   // ESC
    /* 0x02 */ {'1',      '!', },   // 1 !
    /* 0x03 */ {'2',      '@', },   // 2 @
    /* 0x04 */ {'3',      '#', },   // 3 #
    /* 0x05 */ {'4',      '$', },   // 4 $
    /* 0x06 */ {'5',      '%', },   // 5 %
    /* 0x07 */ {'6',      '^', },   // 6 ^
    /* 0x08 */ {'7',      '&', },   // 7 &
    /* 0x09 */ {'8',      '*', },   // 8 *
    /* 0x0A */ {'9',      '(', },   // 9 (
    /* 0x0B */ {'0',      ')', },   // 0 )
    /* 0x0C */ {'-',      '_', },   // - _
    /* 0x0D */ {'=',      '+', },   // = +
    /* 0x0E */ {'\b',     '\b',},   // Backspace
    /* 0x0F */ {'\t',     '\t',},   // Tab
    /* 0x10 */ {'q',      'Q', },   // Q
    /* 0x11 */ {'w',      'W', },   // W
    /* 0x12 */ {'e',      'E', },   // E
    /* 0x13 */ {'r',      'R', },   // R
    /* 0x14 */ {'t',      'T', },   // T
    /* 0x15 */ {'y',      'Y', },   // Y
    /* 0x16 */ {'u',      'U', },   // U
    /* 0x17 */ {'i',      'I', },   // I
    /* 0x18 */ {'o',      'O', },   // O
    /* 0x19 */ {'p',      'P', },   // P
    /* 0x1A */ {'[',      '{', },   // [ {
    /* 0x1B */ {']',      '}', },   // ] }
    /* 0x1C */ {'\n',     '\n',},   // Enter
    /* 0x1D */ {INV,      INV, },   // Left Ctrl (无字符)
    /* 0x1E */ {'a',      'A', },   // A
    /* 0x1F */ {'s',      'S', },   // S
    /* 0x20 */ {'d',      'D', },   // D
    /* 0x21 */ {'f',      'F', },   // F
    /* 0x22 */ {'g',      'G', },   // G
    /* 0x23 */ {'h',      'H', },   // H
    /* 0x24 */ {'j',      'J', },   // J
    /* 0x25 */ {'k',      'K', },   // K
    /* 0x26 */ {'l',      'L', },   // L
    /* 0x27 */ {';',      ':', },   // ; :
    /* 0x28 */ {'\'',     '\"',},   // ' "
    /* 0x29 */ {'`',      '~', },   // ` ~
    /* 0x2A */ {INV,      INV, },   // Left Shift (无字符)
    /* 0x2B */ {'\\',     '|', },   // \ |
    /* 0x2C */ {'z',      'Z', },   // Z
    /* 0x2D */ {'x',      'X', },   // X
    /* 0x2E */ {'c',      'C', },   // C
    /* 0x2F */ {'v',      'V', },   // V
    /* 0x30 */ {'b',      'B', },   // B
    /* 0x31 */ {'n',      'N', },   // N
    /* 0x32 */ {'m',      'M', },   // M
    /* 0x33 */ {',',      '<', },   // , <
    /* 0x34 */ {'.',      '>', },   // . >
    /* 0x35 */ {'/',      '?', },   // / ?
    /* 0x36 */ {INV,      INV, },   // Right Shift (无字符)
    /* 0x37 */ {'*',      '*', },   // 小键盘 *
    /* 0x38 */ {INV,      INV, },   // Left Alt (无字符)
    /* 0x39 */ {' ',      ' ', },   // Space
    /* 0x3A */ {INV,      INV, },   // Caps Lock (无字符)
    /* 0x3B */ {INV,      INV, },   // F1 (无字符)
    /* 0x3C */ {INV,      INV, },   // F2 (无字符)
    /* 0x3D */ {INV,      INV, },   // F3 (无字符)
    /* 0x3E */ {INV,      INV, },   // F4 (无字符)
    /* 0x3F */ {INV,      INV, },   // F5 (无字符)
    /* 0x40 */ {INV,      INV, },   // F6 (无字符)
    /* 0x41 */ {INV,      INV, },   // F7 (无字符)
    /* 0x42 */ {INV,      INV, },   // F8 (无字符)
    /* 0x43 */ {INV,      INV, },   // F9 (无字符)
    /* 0x44 */ {INV,      INV, },   // F10 (无字符)
    /* 0x45 */ {INV,      INV, },   // Num Lock (无字符)
    /* 0x46 */ {INV,      INV, },   // Scroll Lock (无字符)
    /* 0x47 */ {'7',      '7', },   // 小键盘 7 (Home)
    /* 0x48 */ {'8',      '8', },   // 小键盘 8 (↑)
    /* 0x49 */ {'9',      '9', },   // 小键盘 9 (PgUp)
    /* 0x4A */ {'-',      '-', },   // 小键盘 -
    /* 0x4B */ {'4',      '4', },   // 小键盘 4 (←)
    /* 0x4C */ {'5',      '5', },   // 小键盘 5
    /* 0x4D */ {'6',      '6', },   // 小键盘 6 (→)
    /* 0x4E */ {'+',      '+', },   // 小键盘 +
    /* 0x4F */ {'1',      '1', },   // 小键盘 1 (End)
    /* 0x50 */ {'2',      '2', },   // 小键盘 2 (↓)
    /* 0x51 */ {'3',      '3', },   // 小键盘 3 (PgDn)
    /* 0x52 */ {'0',      '0', },   // 小键盘 0 (Ins)
    /* 0x53 */ {'.',      '.', },   // 小键盘 . (Del)
    /* 0x54 */ {INV,      INV, },   // (保留)
    /* 0x55 */ {INV,      INV, },   // (保留)
    /* 0x56 */ {INV,      INV, },   // (保留)
    /* 0x57 */ {INV,      INV, },   // F11 (XT键盘无)
    /* 0x58 */ {INV,      INV, },   // F12 (XT键盘无)
    /* 0x59 */ {INV,      INV, },   // (保留)
    /* 0x5A */ {INV,      INV, },   // (保留)
    /* 0x5B */ {INV,      INV, },   // (保留)
    /* 0x5C */ {INV,      INV, },   // (保留)
    /* 0x5D */ {INV,      INV, },   // (保留)
    /* 0x5E */ {INV,      INV, },   // (保留)

    //强制定义 print screen 为0x5F
    /* 0x5F */ {INV,      INV,},   // PrintScreen
};

static bool ctrl_state, shift_state, caps_lock_state, alt_state, ext_scancode;


static void intr_keyboard_handler(uint8_t vector) {
    // bool ctrl_last = ctrl_state;
    bool shift_last = shift_state;
    bool caps_lock_last = caps_lock_state;
    bool break_code;
    uint16_t scancode = inb(KEYBOARD_PORT);

    //扩展通码
    if (scancode == 0xe0) {
        ext_scancode = true;
        return;
    }

    if (ext_scancode) {
        ext_scancode = false;
        scancode = ((0xe000) | scancode);
    }

    //断码
    break_code = ((scancode & 0x80) != 0);

    //松开按键
    if (break_code) {
        //不处理多字节扫描码
        uint16_t make_code = (scancode &= 0xff7f);
        switch(make_code) {
        case ctrl_l_make:
        case ctrl_r_make:
            ctrl_state = false;
            break;
        case shift_l_make:
        case shift_r_make:
            shift_state = false;
            break;
        case alt_l_make:
        case alt_r_make:
            alt_state = false;
            break;
        }
        return;
    }

    if (scancode >= 0x3b && scancode != alt_r_make && scancode != ctrl_r_make) {
        return;
    }
    
    bool shift = shift_last;
    if (caps_lock_last) shift = !shift;

    uint8_t index = (scancode &= 0x00ff);
    char ch = keymap[index][shift];

    if (ch) {
        if (!ring_full(&kb_buf)) {
            io_ring_putchar(&kb_buf, ch);
            // console_write(&ch, 1);
        }
        return;
    }

     //控制键
    switch(scancode) {
    case ctrl_l_make:
    case ctrl_r_make:
        ctrl_state = true;
        break;
    case shift_l_make:
    case shift_r_make:
        shift_state = true;
        break;
    case alt_l_make:
    case alt_r_make:
        alt_state = true;
        break;
    case caps_lock_make:
        caps_lock_state = !caps_lock_state;
        break;
    }
} 

void keyboard_init() {
    io_ring_init(&kb_buf);
    register_handler(KEYBOARD_VECTOR, intr_keyboard_handler);
}