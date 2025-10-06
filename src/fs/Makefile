CC:= gcc
SS:= nasm
LD:= ld

BUILD:=../../build
SRC_DIR:=..

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include -Wall
CDEBUG:= -g

CPP:= gcc -E -nostdinc -I$(SRC_DIR)/include

SS_FLAGS:= -f elf32 -gdwarf

LD_FLAGS:= -m elf_i386


OBJS:=\
$(BUILD)/fs/buffer.o \
# $(BUILD)/fs/fs.o 

$(BUILD)/fs/%.o: %.c
	$(CC) $(CFLAGS) $(CDEBUG) -c -o $@ $<

$(BUILD)/fs/%.o: %.asm
	$(SS) $(SS_FLAGS) -o $@ $<

$(BUILD)/fs/filesys.o: $(OBJS)
	$(LD) $(LD_FLAGS) -r -o $@ $^


.PHONY: clear
clear: 
	rm -r $(BUILD)/fs/*

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
fs.o: fs.c ../include/fs/fs.h ../include/ide.h ../include/stdint.h \
 ../include/mutex.h ../include/thread.h ../include/list.h \
 ../include/bitmap.h ../include/memory.h ../include/debug.h \
 ../include/assert.h ../include/os.h ../include/string.h
