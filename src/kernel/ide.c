#include <ide.h>
#include <debug.h>
#include <stdio.h>
#include <io.h>
#include <assert.h>
#include <timer.h>
#include <interrupt.h>
#include <os.h>
#include <string.h>
#include <device/dev.h>

#define reg_data(channel)         (channel->port_base + 0) 
#define reg_error(channel)        (channel->port_base + 1) 
#define reg_sect_cnt(channel)   (channel->port_base + 2) 
#define reg_lba_l(channel)       (channel->port_base + 3) 
#define reg_lba_m(channel)        (channel->port_base + 4) 
#define reg_lba_h(channel)       (channel->port_base + 5) 
#define reg_dev(channel)          (channel->port_base + 6) 
#define reg_status(channel)     (channel->port_base + 7) 
#define reg_cmd(channel)           (reg_status(channel)) 
#define reg_alt_status(channel) (channel->port_base + 0x206) 
#define reg_ctl(channel)           (reg_alt_status(channel) )

/* reg_alt_status 寄存器的一些关键位 */ 
#define BIT_ALT_STAT_BSY         0x80    // 硬盘忙 
#define BIT_ALT_STAT_DRDY        0x40    // 驱动器准备好 
#define BIT_ALT_STAT_DRQ         0x8     // 数据传输准备好了



 /* device 寄存器的一些关键位 */ 
#define BIT_DEV_MBS     0xa0       // 第7位和第5位固定为1 
#define BIT_DEV_LBA     0x40       // 寻址模式LBA
#define BIT_DEV_DEV     0x10        // 从盘

/* 一些硬盘操作的指令 */ 
#define CMD_IDENTIFY       0xec    // identify 指令 
#define CMD_READ_SECTOR    0x20   // 读扇区指令 
#define CMD_WRITE_SECTOR   0x30   // 写扇区指令 

/* 定义可读写的最大扇区数，调试用的 */ 
#define HARDDISK_MAX_MEMORY 80
#define max_lba ((HARDDISK_MAX_MEMORY*1024*1024/512) - 1)  // 只支持80MB硬盘 

#define NR_CHANNEL 2
#define NR_HARDDISK 2 * NR_CHANNEL

#define flush_tlb(vaddr) asm volatile("invlpg (%0)"::"r"(vaddr):"memory")

uint8_t channel_cnt;
ide_channel_t channels[NR_CHANNEL];

int32_t ext_lba_base = 0;   //总扩展分区起始lba
uint8_t partition_no = 0, logical_no = 0;

list_t partition_list;

typedef struct partition_table_entry_t {
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sec;
    uint8_t start_chs;
    uint8_t fs_type;    //分区类型
    uint8_t end_head;
    uint8_t end_sec;
    uint8_t end_chs;
    uint32_t start_lba;
    uint32_t sec_cnt;
} _packed partition_table_entry_t;

//引导扇区
typedef struct boot_sector_t {
    uint8_t other[446];
    partition_table_entry_t partition_table[4];
    uint16_t signature; //55,aa
} _packed boot_sector_t;

void intr_hd_handler(uint8_t vector) {
    assert(vector == 0x2e || vector == 0x2f);
    const uint8_t ch_no = vector - 0x2e;
    ide_channel_t* ide = &channels[ch_no];

    if (ide->expecting_intr) {
        ide->expecting_intr = false;
        sema_up(&ide->disk_done);
        inb(reg_status(ide));
    }
}

//等待中断
static inline void wait_for_interrupt(ide_channel_t* ide) {
    sema_down(&ide->disk_done);
}

//len个相邻字节交换位置后存入buf
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
    uint8_t i;
    for (i = 0;i < len;i += 2) {
        buf[i + 1] = *dst++;
        buf[i] = *dst++;
    }
    buf[i] = '\0';
}
 
static void select_disk(disk_t* hd) {
    uint8_t device = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->dev_no == 1) {
        device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->ide), device);
}

static void select_sector(disk_t* hd, uint32_t lba, uint8_t sec_cnt) {
    assert(lba <= max_lba);
    ide_channel_t* ide = hd->ide;
    outb(reg_sect_cnt(ide), sec_cnt);

    outb(reg_lba_l(ide), lba      );
    outb(reg_lba_m(ide), lba >> 8 );
    outb(reg_lba_h(ide), lba >> 16);
    outb(reg_dev(ide), BIT_DEV_MBS | BIT_DEV_LBA |
        (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | (lba >> 24));
}

static void cmd_out(ide_channel_t* ide, uint8_t cmd) {
    ide->expecting_intr = true;
    outb(reg_cmd(ide), cmd);
}

static void read_from_sector(disk_t* hd, void* buf) {
    _insw(reg_data(hd->ide), buf, SECTOR_SIZE / 2);
}

static void write_to_sector(disk_t* hd, void* buf) {
    _outsw(reg_data(hd->ide), buf, SECTOR_SIZE / 2);
}

static bool busy_wait(disk_t* hd) {
    ide_channel_t* ide = hd->ide;
    uint8_t status;
    while (1) {
        status = inb(reg_status(ide));
        if (!(status & BIT_ALT_STAT_BSY)) {
            //不是忙
            return (status & BIT_ALT_STAT_DRQ);
        }
        else {
            for (size_t i = 10;i > 0;i--)
                ;
        }
    }
    return false;
}

//读取sec_cnt个扇区
void ide_read(disk_t* hd, uint32_t lba, void* buf, uint32_t sec_cnt, int flag) {
    assert(lba <= max_lba);
    assert(sec_cnt > 0);
    lock_acquire(&hd->ide->lock);

    select_disk(hd);
    uint32_t secs_op;   //每次操作的扇区数
    uint32_t secs_done = 0; //完成的扇区数
    while (secs_done < sec_cnt) {
        if (secs_done + 256 <= sec_cnt) {
            secs_op = 256;
        }
        else {
            secs_op = sec_cnt - secs_done;
        }
        select_sector(hd, lba + secs_done, secs_op);
        cmd_out(hd->ide, CMD_READ_SECTOR);

        for (size_t i = 0;i < secs_op;i++) {
            hd->ide->expecting_intr = true;
            if (running_task()->status == TASK_RUNNING) {
                //系统初始化时不用异步
                //只有非初始化时候才会进入这里
                sema_down(&hd->ide->disk_done);
            }
            busy_wait(hd);
            read_from_sector(hd, (void*)((uint32_t)buf + i * 512));
        }
        secs_done += secs_op;
    }
    lock_release(&hd->ide->lock);
}

void ide_write(disk_t* hd, uint32_t lba, void* buf, uint32_t sec_cnt, int flag) {
    assert(lba <= max_lba);
    assert(sec_cnt > 0);
    lock_acquire(&hd->ide->lock);

    select_disk(hd);
    uint32_t secs_op;   //每次操作的扇区数
    uint32_t secs_done = 0; //完成的扇区数
    while (secs_done < sec_cnt) {
        if (secs_done + 256 <= sec_cnt) {
            secs_op = 256;
        }
        else {
            secs_op = sec_cnt - secs_done;
        }
        select_sector(hd, lba + secs_done, secs_op);
        cmd_out(hd->ide, CMD_WRITE_SECTOR);

        /*中断处理程序唤醒后执行*/
        if (!busy_wait(hd)) {       //失败，不可读
            char error[64];
            sprintf(error, "%s, read sector %d fail\n", hd->name, lba);
            panic(error);
        }
        
        for (size_t i = 0;i < secs_op;i++) {
            hd->ide->expecting_intr = true;
            write_to_sector(hd, (void*)((uint32_t)buf + i * 512));
            if (running_task()->status == TASK_RUNNING) {
                //系统初始化时不用异步
                //只有非初始化时候才会进入这里
                sema_down(&hd->ide->disk_done);
            }
            busy_wait(hd);
        }
        
        secs_done += secs_op;
    }
    lock_release(&hd->ide->lock);
}

void ide_part_read(partition_t *part, uint32_t lba, void* buf, uint32_t sec_cnt, int flag) {
    ide_read(part->disk, lba, buf, sec_cnt, flag);
}

void ide_part_write(partition_t *part, uint32_t lba, void* buf, uint32_t sec_cnt, int flag) {
    ide_write(part->disk, lba, buf, sec_cnt, flag);
}

//获取硬盘参数
static void identify_disk(disk_t* hd) {
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->ide, CMD_IDENTIFY);
    // sema_down(&hd->ide->disk_done);

    if (!busy_wait(hd)) {       //失败，不可读
        char error[64];
        sprintf(error, "%s identify fail\n", hd->name);
        panic(error);
    }
    read_from_sector(hd, id_info);
    
    char buf[64];
    uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
    swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
    printk("disk %s info: \nSN: %s\n", hd->name, buf);
    memset(buf, 0, sizeof(buf));
    swap_pairs_bytes(&id_info[md_start], buf, md_len);
    printk("Module: %s\n", buf);
    uint32_t sectors = *(uint32_t*)&id_info[60 * 2];
    printk("Sectors: %d\n", sectors);
    printk("Capacity: %dMB\n", sectors * 512 / 1024 / 1024);
}

static void partition_scan(disk_t *hd, uint32_t ext_lba) {
    boot_sector_t* bs = kmalloc(sizeof(boot_sector_t));
    ide_read(hd, ext_lba, bs, 1, 0);
    uint8_t i = 0;
    partition_table_entry_t* p = bs->partition_table;
    while (i++ < 4) {
        if (p->fs_type == 0x5) { //扩展分区
            if (ext_lba_base != 0) {
                partition_scan(hd, p->start_lba + ext_lba_base);
            } 
            else {  //第一次读取引导块, 即主引导
                ext_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            }
        }
        else if (p->fs_type != 0) {
            //有效分区
            if (ext_lba == 0) { //主分区
                hd->parts[partition_no].start_lba = ext_lba + p->start_lba;
                hd->parts[partition_no].sector_cnt = p->sec_cnt;
                hd->parts[partition_no].disk = hd;
                list_pushback(&partition_list, &hd->parts[partition_no].part_node);
                sprintf(hd->parts[partition_no].name, "%s%d", hd->name, partition_no + 1);
                partition_no++;
            }
            else {
                hd->logical_parts[logical_no].start_lba = ext_lba + p->start_lba;
                hd->logical_parts[logical_no].sector_cnt = p->sec_cnt;
                hd->logical_parts[logical_no].disk = hd;
                list_pushback(&partition_list, &hd->logical_parts[logical_no].part_node);
                sprintf(hd->logical_parts[logical_no].name, "%s%d", hd->name, logical_no + 5);
                logical_no++;
                if (logical_no >= 8) {
                    return;
                }
            }
        }
        p++;
    }
    kfree(bs);
}

static void partition_info(list_node_t* elem) {
    partition_t* part = element_entry(partition_t, part_node, elem);
    printk("%s start_lba: %d, sec_cnt: %d\n", part->name, part->start_lba, part->sector_cnt);
}


//安装设备
static void ide_device_install() {
    //TODO
    for (size_t i = 0;i < channel_cnt;i++) {
        for (size_t j = 0;j < 2;j++) {
            //硬盘不存在
            if (channels[i].disks[j].dev_no == -1) continue;
            //安装硬盘
            disk_t *hd = &channels[i].disks[j];
            dev_t hd_dev = device_install(
                hd->name, HARDDISK_PARENT, DEV_BLOCK, DEV_IDE_HARDDISK,
                (void*)hd, NULL, ide_read, ide_write);
            for (size_t k = 0;k < NR_MAIN_PART;k++) {
                partition_t* part = &hd->parts[k];
                if (!part->disk) continue;
                device_install(part->name, hd_dev, DEV_BLOCK, DEV_IDE_PART,
                    (void*)part, NULL, ide_part_read, ide_part_write);
            }
            for (size_t k = 0;k < NR_LOGIC_PART;k++) {
                partition_t* part = &hd->logical_parts[k];
                if (!part->disk) continue;
                device_install(part->name, hd_dev, DEV_BLOCK, DEV_IDE_PART,
                    (void*)part, NULL, ide_part_read, ide_part_write);
            }
        }
    }
}

void ide_init() {
    LOGK("IDE Init...");
    list_init(&partition_list);

    //需要读取系统引导块的内容
    uint32_t* boot_sector_pte = (uint32_t*)0xFFC00000;
    *boot_sector_pte = 7;
    flush_tlb(0);

    const uint8_t hd_cnt = *((uint8_t*)0x475);
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

    ide_channel_t* channel;
    uint8_t channel_no = 0;

    //读入信息之前先初始化硬盘为空
    for (size_t i = 0;i < NR_CHANNEL;i++) {
        for (size_t j = 0;j < 2;j++) {
            channels[i].disks[j].dev_no = -1;
        }
    }


    for (;channel_no < channel_cnt;channel_no++) {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);
        switch (channel_no) {
        case 0:
            channel->port_base = 0x1f0;
            channel->irq_no = 0x20 + 14;
            break;
        case 1:
            channel->port_base = 0x170;
            channel->irq_no = 0x20 + 15;
            break;
        }
        channel->expecting_intr = false;
        lock_init(&channel->lock);
        sema_init(&channel->disk_done, 0);
        register_handler(channel->irq_no, intr_hd_handler);
        for (int dev_no = 0;dev_no < 2;dev_no++) {  //遍历硬盘
            disk_t *hd = &channel->disks[dev_no];
            hd->ide = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "hd%c", 'a' + channel_no * 2 + dev_no);
            identify_disk(hd);
            if (!(channel_no == 0 && dev_no == 0)) {    //不是内核盘
                partition_scan(hd, 0);
            }
            partition_no = 0, logical_no = 0;
        }
    }
    LOGK("\nall partition info");
    list_node_t* node = partition_list.head.next;
    for (; node != &partition_list.tail;node = node->next) {
        partition_info(node);
    }

    ide_device_install();

    *boot_sector_pte = 0;
    flush_tlb(0);

    LOGK("\npartition done");
}