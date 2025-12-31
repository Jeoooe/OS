#ifndef OS_ELF_H
#define OS_ELF_H

#include <stdint.h>
#define EI_NIDENT 16

/*  Elf头相关宏 */
//魔数相关下标
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_PAD      7

//e_type
#define ET_NONE     0   //未知格式
#define ET_REL      1   //可重定位文件
#define ET_EXEC     2   //可执行文件
#define ET_DYN      3
#define ET_CORE     4
#define ET_LOPROC   0xff00
#define ET_HIPROC   0xffff

//e_machine
#define ET_NONE     0   //未知架构
#define EM_386      3   //Intel 80386架构

//e_version
#define EV_NONE     0   //非法版本
#define EV_CURRENT  1   //当前版本

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

//Elf 文件头结构
typedef struct Elf32_Ehdr{
    unsigned char e_ident[EI_NIDENT];  // ELF魔数、版本等
    Elf32_Half    e_type;              // 文件类型
    Elf32_Half    e_machine;           // 目标机器架构
    Elf32_Word    e_version;           // 版本
    Elf32_Addr    e_entry;             // 程序入口点（最重要）
    Elf32_Off     e_phoff;             // Program Headers偏移
    Elf32_Off     e_shoff;             // Section Headers偏移
    Elf32_Word    e_flags;             // 处理器特定标志
    Elf32_Half    e_ehsize;            // ELF头大小
    Elf32_Half    e_phentsize;         // Program Header大小
    Elf32_Half    e_phnum;             // Program Header数量
    Elf32_Half    e_shentsize;         // Section Header大小
    Elf32_Half    e_shnum;             // Section Header数量
    Elf32_Half    e_shstrndx;          // 节名字符串表索引
} Elf32_Ehdr;

//p_type    
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2

//程序头表  Program Header Table
typedef struct Elf32_Phdr {
	Elf32_Word p_type;      //当前段类型
	Elf32_Off p_offset;     //段相对文件开头偏移量
	Elf32_Addr p_vaddr;     //段第一个字节映射到内存的虚拟地址
	Elf32_Addr p_paddr;     //使用虚拟地址的系统忽略
	Elf32_Word p_filesz;    //段在文件映像中占用字节数
	Elf32_Word p_memsz;     //段在内存映像中占用字节数
	Elf32_Word p_flags;     //段相关标志
	Elf32_Word p_align;     //段的对齐方式
} Elf32_Phdr;


#endif