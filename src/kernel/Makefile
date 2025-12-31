CC:= gcc
SS:= nasm
LD:= ld

BUILD:=../../build/kernel
SRC_DIR:=..

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include -Wall
CDEBUG:= -g

CPP:= gcc -E -nostdinc -I$(SRC_DIR)/include

SS_FLAGS:= -f elf32 -gdwarf

LD_FLAGS:= -m elf_i386


OBJS:=\
$(BUILD)/start.o \
$(BUILD)/main.o \
$(BUILD)/io.o \
$(BUILD)/console.o \
$(BUILD)/printk.o \
$(BUILD)/assert.o \
$(BUILD)/debug.o \
$(BUILD)/interrupt.o \
$(BUILD)/interrupt_handler.o \
$(BUILD)/interrupt_state.o \
$(BUILD)/timer.o \
$(BUILD)/memory.o \
$(BUILD)/bitmap.o \
$(BUILD)/thread.o \
$(BUILD)/switch.o \
$(BUILD)/tss.o \
$(BUILD)/syscall.o \
$(BUILD)/sysc.o \
$(BUILD)/exit.o \
$(BUILD)/ide.o \
$(BUILD)/setup.o \
$(BUILD)/mktime.o \
$(BUILD)/system.o \
$(BUILD)/signal.o \

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $(CDEBUG) -c -o $@ $<

$(BUILD)/%.o: %.asm
	$(SS) $(SS_FLAGS) -o $@ $<

$(BUILD)/kernel.o: $(OBJS)
	$(LD) $(LD_FLAGS) -r -o $@ $^


.PHONY: clear
clear: 
	rm -r $(BUILD)/*
dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i | sed 's|^\(.*\)\.o:|../../build/kernel/\1.o:|';done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
../../build/kernel/assert.o: assert.c ../include/os.h ../include/assert.h
../../build/kernel/bitmap.o: bitmap.c ../include/bitmap.h ../include/stdint.h \
 ../include/string.h ../include/assert.h ../include/debug.h
../../build/kernel/console.o: console.c ../include/stdint.h ../include/stdio.h \
 ../include/stdarg.h ../include/io.h ../include/debug.h \
 ../include/mutex.h ../include/list.h
../../build/kernel/debug.o: debug.c ../include/debug.h ../include/os.h ../include/stdarg.h \
 ../include/stdio.h
../../build/kernel/exit.o: exit.c ../include/thread.h ../include/stdint.h \
 ../include/signal.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/debug.h ../include/assert.h
../../build/kernel/ide.o: ide.c ../include/ide.h ../include/stdint.h ../include/mutex.h \
 ../include/list.h ../include/thread.h ../include/signal.h \
 ../include/bitmap.h ../include/memory.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/debug.h ../include/stdio.h \
 ../include/stdarg.h ../include/io.h ../include/assert.h \
 ../include/timer.h ../include/time.h ../include/interrupt.h \
 ../include/os.h ../include/string.h ../include/device/dev.h
../../build/kernel/interrupt.o: interrupt.c ../include/os.h ../include/interrupt.h \
 ../include/stdint.h ../include/debug.h ../include/io.h \
 ../include/global.h ../include/assert.h
../../build/kernel/main.o: main.c ../include/os.h ../include/interrupt.h ../include/stdint.h \
 ../include/debug.h ../include/thread.h ../include/signal.h \
 ../include/list.h ../include/bitmap.h ../include/memory.h \
 ../include/fs/fs.h ../include/fs/buffer.h ../include/mutex.h \
 ../include/syscall.h ../include/fs/fcntl.h ../include/device/dev.h
../../build/kernel/memory.o: memory.c ../include/os.h ../include/memory.h \
 ../include/stdint.h ../include/bitmap.h ../include/list.h \
 ../include/string.h ../include/debug.h ../include/assert.h \
 ../include/thread.h ../include/signal.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/mutex.h ../include/interrupt.h
../../build/kernel/mktime.o: mktime.c ../include/time.h ../include/stdint.h ../include/io.h
../../build/kernel/printk.o: printk.c ../include/os.h ../include/stdarg.h \
 ../include/console.h ../include/stdint.h
../../build/kernel/setup.o: setup.c ../include/os.h ../include/debug.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/stdint.h ../include/mutex.h \
 ../include/list.h ../include/fs/stat.h ../include/fs/fcntl.h \
 ../include/syscall.h ../include/thread.h ../include/signal.h \
 ../include/bitmap.h ../include/memory.h ../include/device/dev.h
../../build/kernel/signal.o: signal.c ../include/signal.h ../include/stdint.h \
 ../include/thread.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/interrupt.h
../../build/kernel/syscall.o: syscall.c ../include/syscall.h ../include/stdint.h \
 ../include/debug.h ../include/thread.h ../include/signal.h \
 ../include/list.h ../include/bitmap.h ../include/memory.h \
 ../include/fs/fs.h ../include/fs/buffer.h ../include/mutex.h \
 ../include/stdio.h ../include/stdarg.h ../include/console.h
../../build/kernel/system.o: system.c ../include/thread.h ../include/stdint.h \
 ../include/signal.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h
../../build/kernel/thread.o: thread.c ../include/thread.h ../include/stdint.h \
 ../include/signal.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/string.h ../include/os.h \
 ../include/interrupt.h ../include/assert.h ../include/debug.h \
 ../include/stdio.h ../include/stdarg.h ../include/tss.h
../../build/kernel/timer.o: timer.c ../include/timer.h ../include/stdint.h ../include/time.h \
 ../include/io.h ../include/debug.h ../include/thread.h \
 ../include/signal.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/assert.h ../include/os.h \
 ../include/interrupt.h
../../build/kernel/tss.o: tss.c ../include/stdint.h ../include/global.h ../include/thread.h \
 ../include/signal.h ../include/list.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/debug.h ../include/string.h
