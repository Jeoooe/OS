CC:= gcc
SS:= nasm
LD:= ld

#目标路径
BUILD:=../../build/userprog
SRC_DIR:=..

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include -Wall
CDEBUG:= -g

CPP:= gcc -E -nostdinc -I$(SRC_DIR)/include

SS_FLAGS:= -f elf32 -gdwarf

LD_FLAGS:= -m elf_i386


OBJS:=\
$(BUILD)/process.o \
$(BUILD)/printf.o \

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $(CDEBUG) -c -o $@ $<

$(BUILD)/%.o: %.asm
	$(SS) $(SS_FLAGS) -o $@ $<

$(BUILD)/userprog.o: $(OBJS)
	$(LD) $(LD_FLAGS) -r -o $@ $^


.PHONY: clear
clear: 
	rm -r $(BUILD)/*

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
printf.o: printf.c ../include/stdio.h ../include/stdarg.h \
 ../include/syscall.h ../include/stdint.h
process.o: process.c ../include/interrupt.h ../include/stdint.h \
 ../include/userprog.h ../include/thread.h ../include/list.h \
 ../include/bitmap.h ../include/memory.h ../include/global.h \
 ../include/assert.h ../include/tss.h ../include/string.h ../include/os.h \
 ../include/debug.h
