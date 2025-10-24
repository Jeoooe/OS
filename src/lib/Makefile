CC:= gcc
SS:= nasm
LD:= ld

#目标路径
BUILD:=../../build/lib
SRC_DIR:=..

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include -Wall
CDEBUG:= -g

CPP:= gcc -E -nostdinc -I$(SRC_DIR)/include

SS_FLAGS:= -f elf32 -gdwarf

LD_FLAGS:= -m elf_i386


OBJS:=\
$(BUILD)/errno.o \
$(BUILD)/string.o \
$(BUILD)/vsprintf.o \
$(BUILD)/list.o \
$(BUILD)/mutex.o 

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $(CDEBUG) -c -o $@ $<

$(BUILD)/%.o: %.asm
	$(SS) $(SS_FLAGS) -o $@ $<

$(BUILD)/lib.o: $(OBJS)
	$(LD) $(LD_FLAGS) -r -o $@ $^


.PHONY: clear
clear: 
	rm -r $(BUILD)/*

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
list.o: list.c ../include/list.h ../include/stdint.h ../include/assert.h
mutex.o: mutex.c ../include/mutex.h ../include/stdint.h \
 ../include/thread.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/interrupt.h ../include/assert.h
string.o: string.c ../include/string.h ../include/stdint.h
vsprintf.o: vsprintf.c ../include/stdarg.h ../include/string.h \
 ../include/stdint.h ../include/assert.h ../include/stdio.h
