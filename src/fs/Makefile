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
$(BUILD)/fs/bitmap.o \
$(BUILD)/fs/buffer.o \
$(BUILD)/fs/fs.o \
$(BUILD)/fs/inode.o \
$(BUILD)/fs/namei.o \
$(BUILD)/fs/open.o \
$(BUILD)/fs/super.o \
$(BUILD)/fs/truncate.o \
$(BUILD)/fs/read_write.o \
$(BUILD)/fs/file_dev.o \
$(BUILD)/fs/fcntl.o \
$(BUILD)/fs/exec.o \

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
	(for i in *.c;do $(CPP) -M $$i | sed 's|^\(.*\)\.o:|../../build/fs/\1.o:|';done) >> tmp_make
	cp tmp_make Makefile

### Dependencies
../../build/fs/bitmap.o: bitmap.c ../include/os.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/stdint.h ../include/mutex.h \
 ../include/list.h ../include/thread.h ../include/signal.h \
 ../include/bitmap.h ../include/memory.h ../include/time.h \
 ../include/string.h ../include/assert.h
../../build/fs/buffer.o: buffer.c ../include/fs/buffer.h ../include/stdint.h \
 ../include/mutex.h ../include/list.h ../include/device/dev.h \
 ../include/thread.h ../include/signal.h ../include/bitmap.h \
 ../include/memory.h ../include/fs/fs.h ../include/ide.h \
 ../include/assert.h ../include/os.h
../../build/fs/exec.o: exec.c ../include/fs/elf.h ../include/stdint.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/mutex.h ../include/list.h \
 ../include/fs/fcntl.h ../include/debug.h
../../build/fs/fcntl.o: fcntl.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/fs/stat.h ../include/thread.h ../include/signal.h \
 ../include/bitmap.h ../include/memory.h ../include/errno.h
../../build/fs/file_dev.o: file_dev.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/fs/fcntl.h ../include/errno.h ../include/time.h
../../build/fs/fs.o: fs.c ../include/fs/fs.h ../include/fs/buffer.h ../include/stdint.h \
 ../include/mutex.h ../include/list.h
../../build/fs/inode.o: inode.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/time.h ../include/string.h ../include/debug.h \
 ../include/assert.h
../../build/fs/namei.o: namei.c ../include/os.h ../include/assert.h ../include/thread.h \
 ../include/stdint.h ../include/signal.h ../include/list.h \
 ../include/bitmap.h ../include/memory.h ../include/fs/fs.h \
 ../include/fs/buffer.h ../include/mutex.h ../include/fs/stat.h \
 ../include/fs/fcntl.h ../include/errno.h ../include/time.h \
 ../include/string.h
../../build/fs/open.o: open.c ../include/fs/fcntl.h ../include/stdint.h \
 ../include/fs/stat.h ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/mutex.h ../include/list.h ../include/errno.h \
 ../include/thread.h ../include/signal.h ../include/bitmap.h \
 ../include/memory.h ../include/assert.h
../../build/fs/read_write.o: read_write.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/fs/stat.h ../include/errno.h ../include/thread.h \
 ../include/signal.h ../include/bitmap.h ../include/memory.h \
 ../include/device/dev.h
../../build/fs/super.o: super.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/fs/stat.h ../include/device/dev.h ../include/thread.h \
 ../include/signal.h ../include/bitmap.h ../include/memory.h \
 ../include/ide.h ../include/string.h ../include/os.h ../include/debug.h \
 ../include/assert.h
../../build/fs/truncate.o: truncate.c ../include/fs/fs.h ../include/fs/buffer.h \
 ../include/stdint.h ../include/mutex.h ../include/list.h \
 ../include/fs/stat.h ../include/time.h
