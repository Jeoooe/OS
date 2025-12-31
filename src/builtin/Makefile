CC:= gcc
SS:= nasm
LD:= ld

#目标路径
BUILD:=../../build/device
SRC_DIR:=..

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include -Wall
CDEBUG:= -g

CPP:= gcc -E -nostdinc -I$(SRC_DIR)/include

SS_FLAGS:= -f elf32

LD_FLAGS:= -m elf_i386

hello.out: hello.o
	$(LD) $(LD_FLAGS) -o $@ $<

hello.o: hello.asm
	$(SS) $(SS_FLAGS) -o $@ $<
