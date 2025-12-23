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

SS_FLAGS:= -f elf32 -gdwarf

LD_FLAGS:= -m elf_i386


OBJS:=\
$(BUILD)/dev.o \
$(BUILD)/keyboard.o \
$(BUILD)/ring.o \
$(BUILD)/tty.o \


$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $(CDEBUG) -c -o $@ $<

$(BUILD)/%.o: %.asm
	$(SS) $(SS_FLAGS) -o $@ $<

$(BUILD)/device.o: $(OBJS)
	$(LD) $(LD_FLAGS) -r -o $@ $^


.PHONY: clear
clear: 
	rm -r $(BUILD)/*

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
keyboard.o: keyboard.c ../include/stdint.h ../include/interrupt.h \
 ../include/os.h ../include/io.h ../include/console.h ../include/ring.h \
 ../include/mutex.h ../include/thread.h ../include/list.h \
 ../include/bitmap.h ../include/memory.h
ring.o: ring.c ../include/ring.h ../include/mutex.h ../include/stdint.h \
 ../include/thread.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/assert.h ../include/interrupt.h
