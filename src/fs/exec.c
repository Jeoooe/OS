#include <stdint.h>
#include <string.h>
#include <fs/elf.h>
#include <fs/fs.h>
#include <fs/fcntl.h>
#include <thread.h>
#include <interrupt.h>

#define NDEBUG
#include <debug.h>

#ifndef likely
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

extern int sys_close(uint32_t fd);
extern inode_t* namei(const char* pathname);
extern void get_empty_page(uint32_t vaddr);         //memory.c
extern void release_memory(task_block_t* task, bool preserve_pd);
extern void free_page_directory();

static void print_phdr(Elf32_Phdr *phdr) {
    LOGK("Type:       %d", phdr->p_type);
    LOGK("Offset:   0x%x", phdr->p_offset);
    LOGK("VirtAddr: 0x%x", phdr->p_vaddr);
    LOGK("FileSiz:  0x%x", phdr->p_filesz);
    LOGK("MemSiz:   0x%x", phdr->p_memsz);
    LOGK("Flg:        %d", phdr->p_flags);
    LOGK("Align:    0x%x\n", phdr->p_align);
}


//从文件句柄fd中读取elf文件
//buf必须有效
static int read_elf(inode_t *inode, uint32_t *entry) {
    task_block_t *cur;
    buffer_t *bh = bread(inode->dev, inode->zones[0]);
    if (!bh) {
        goto rollback1;
    }
    Elf32_Ehdr* elf_header = (Elf32_Ehdr *)bh->data;
    //检查是否是elf
    if (elf_header->e_ident[EI_MAG0] != 0x7f ||
        elf_header->e_ident[EI_MAG1] != 'E'  ||
        elf_header->e_ident[EI_MAG2] != 'L'  ||
        elf_header->e_ident[EI_MAG3] != 'F') {
        return -1;
    }
    //要求32位小端合法
    if (elf_header->e_ident[EI_CLASS] != 1 || 
        elf_header->e_ident[EI_DATA] != 1 ||
        elf_header->e_ident[EI_VERSION] != 1) {
        return -1;
    }

    //检查是否是i386架构
    if (elf_header->e_machine != EM_386) {
        return -1;
    }

    //是否可执行
    if (elf_header->e_type != ET_EXEC) return -1;
    //版本是否合法
    if (elf_header->e_version != EV_CURRENT) return -1;

    //检查elf文件格式和内核处理是否匹配
    if (sizeof(Elf32_Phdr) != elf_header->e_phentsize) return -1;

    //到这里就是本系统可以处理的elf文件

    LOGK("Elf read:\ne_type: %d\ne_machine: %d", elf_header->e_type, elf_header->e_machine);
    LOGK("e_entry: 0x%x", elf_header->e_entry);
    LOGK("e_phoff: 0x%x", elf_header->e_phoff);
    LOGK("e_shoff: 0x%x", elf_header->e_shoff);
    LOGK("e_flags: 0x%x", elf_header->e_flags);
    LOGK("e_ehsize: 0x%x", elf_header->e_ehsize);
    LOGK("e_phentsize: 0x%x", elf_header->e_phentsize);
    LOGK("e_phnum: %d", elf_header->e_phnum);
    LOGK("she_entsize: 0x%x", elf_header->e_shentsize);
    LOGK("e_shnum: %d", elf_header->e_shnum);
    LOGK("e_strndx: 0x%x\n", elf_header->e_shstrndx);

    //加载程序头表
    cur = running_task();
    uint32_t block_offset = elf_header->e_phoff % BLOCK_SIZE;
    int block = elf_header->e_phoff / BLOCK_SIZE;
    if (block) { //如果不在第0个块
        brelse(bh);
        int i;
        if (!(i = get_block(inode, block))) {
            return -1;
        }
        if (!(bh = bread(inode->dev, i))) {
            goto rollback1;
        }
    }
    //整个程序头表的大小
    uint32_t phdr_num = elf_header->e_phnum;
    uint32_t i = 0;
    Elf32_Phdr *phdr_array = (Elf32_Phdr *)kmalloc(elf_header->e_phnum * elf_header->e_phentsize);
    //然后从文件里复制过来
    while (phdr_num-- > 0) {
        //这里应该要判断是否是PT_LOAD段
        Elf32_Phdr *phdr = (Elf32_Phdr *)(bh->data + block_offset);
        if (phdr->p_type == PT_LOAD) {
            phdr_array[i++] = *phdr;
            print_phdr(phdr);
        }
        block_offset += sizeof(Elf32_Phdr);
        //下面边界情况一般很少可能发生
        if (unlikely(block_offset >= BLOCK_SIZE - sizeof(Elf32_Phdr) && phdr_num))  {
            //超出了文件范围
            block_offset += sizeof(Elf32_Phdr) - BLOCK_SIZE;
            block++;
            if (block_offset) {
                //如果是两个块交界处
                memcpy(phdr_array, bh->data + BLOCK_SIZE - sizeof(Elf32_Phdr) + block_offset, sizeof(Elf32_Phdr) - block_offset);
                brelse(bh);
                int i;
                if (!(i = get_block(inode, block))) return -1;
                if (!(bh = bread(inode->dev, i))) goto rollback1;
                memcpy(phdr_array + sizeof(Elf32_Phdr) - block_offset, bh->data, block_offset);
                phdr_array++;
            } else {
                *phdr_array++ = *(Elf32_Phdr *)(bh->data - sizeof(Elf32_Phdr));
                brelse(bh);
                int i;
                if (!(i = get_block(inode, block))) return -1;
                if (!(bh = bread(inode->dev, i))) goto rollback1;
            }
            phdr_num--;
        }
    }
    //加入进程头
    cur->exec_phdr_list.length = i;
    cur->exec_phdr_list.array = (void *)phdr_array;

    //好像没有要干的了
    //程序的加载就交给缺页异常吧
    *entry = elf_header->e_entry;

    brelse(bh);
    return 0;
rollback1:
    brelse(bh);
    return -1;
}

//修改进程信息
static void setup_thread(uint32_t entry) {
    task_block_t *cur = running_task();
    //关闭文件
    for (int i = 0;i < NR_OPEN; i++) {
        if (cur->close_on_exec & (1 << i)) {
            sys_close(i);
        }
    }
    //设置中断栈帧
    interrupt_stack_t *intr_frame = (interrupt_stack_t *)((void *)cur + PAGE_SIZE - sizeof(interrupt_stack_t));
    intr_frame->eip = (void (*)(void))entry;
    intr_frame->eflags |= 0x200;    //开启中断
    //用户栈寄存器esp不在这里设置
    
    //设置brk 为虚拟地址最大的段的末尾
    cur->brk = 0;
    Elf32_Phdr *p = (Elf32_Phdr *)cur->exec_phdr_list.array;
    for (int i = 0;i < cur->exec_phdr_list.length;i++) {
        cur->brk = MAX(p->p_vaddr + p->p_memsz, cur->brk);
        p++;
    }
    cur->brk = (cur->brk + PAGE_SIZE - 1) & 0xFFFFF000; //页对齐

    //清除内存页面
    release_memory(cur, true);
    //清除用户部分的页目录
    //但保留内核内存的页目录
    free_page_directory();
    //内核内存的页目录保留, 这里的函数都在进程栈里面, 不会受到影响, 可以放心清除.
}

//设置用户栈, 即入口传参
static void set_user_stack(int argc, char *const argv[], char *const envp[]) {
    task_block_t *cur = running_task();
    interrupt_stack_t *intr_frame = (interrupt_stack_t *)((void *)cur + PAGE_SIZE - sizeof(interrupt_stack_t));
    void *stack = (uint32_t *)(USER_STACK_REAL_TOP);
    stack -= 12;
    //设置入口栈顶, 应该有三个参数 argc, argv, envp
    intr_frame->esp = (uint32_t)stack;
}

int sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    /* 1. 检验是否是elf文件
     * 2. 读取文件头, 以及各段信息
     * 3. 加载用户栈, 参数和环境变量
     * 4. 修改进程信息
     * 5. 返回到执行区域
     * 
     * 目前只实现静态链接程序
    */    
    inode_t *inode = namei(filename);
    uint32_t entry;
    if (!inode) {
        goto rollback0;
    }
    if (read_elf(inode, &entry) != 0) goto rollback0;
    //已经加载完程序头了
    //修改进程信息
    running_task()->exe_file = inode;
    setup_thread(entry);
    //设置用户栈
    set_user_stack(0, argv, envp);
    //结束, iret会跳转到程序入口地址
    //后续交给缺页异常按需加载
    return 0;

rollback0:
    return -1;
}