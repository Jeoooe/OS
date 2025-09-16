ENTRY_POINT:=0xc0010000

BUILD:=./build
SRC_DIR:=./src

INCLUDE_DIR:=$(SRC_DIR)/include

LD_FLAGS:= -m elf_i386 -static -Ttext $(ENTRY_POINT)

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include
CDEBUG:= -g



SYSTEM:=$(BUILD)/system.bin

KERNEL_TARGETS:= \
$(BUILD)/kernel/start.o \
$(BUILD)/kernel/main.o \
$(BUILD)/kernel/io.o \
$(BUILD)/kernel/console.o \
$(BUILD)/kernel/printk.o \
$(BUILD)/kernel/vsprintf.o \
$(BUILD)/kernel/assert.o \
$(BUILD)/kernel/debug.o \
$(BUILD)/kernel/interrupt.o \
$(BUILD)/kernel/interrupt_handler.o \
$(BUILD)/kernel/timer.o \
$(BUILD)/kernel/memory.o \
$(BUILD)/kernel/gdt.o \
$(BUILD)/kernel/bitmap.o \
$(BUILD)/kernel/thread.o \
$(BUILD)/kernel/switch.o \
$(BUILD)/kernel/tss.o \


LIB_TARGETS:= \
$(BUILD)/lib/string.o \
$(BUILD)/lib/list.o \
$(BUILD)/lib/mutex.o \

DEVICE_TARGETS:= \
$(BUILD)/device/keyboard.o \
$(BUILD)/device/ring.o \

USERPROG_TARGETS:= \
$(BUILD)/userprog/process.o \

ALL_TARGETS:= $(KERNEL_TARGETS) $(LIB_TARGETS) $(DEVICE_TARGETS) $(USERPROG_TARGETS)



.PHONY: default
default: $(BUILD)/master.img

$(BUILD)/boot/%.bin: $(SRC_DIR)/boot/%.asm
	nasm -f bin -o $@ $<

$(BUILD)/kernel/%.o: $(SRC_DIR)/kernel/%.asm
	nasm -f elf32 -gdwarf -o $@ $<


$(BUILD)/kernel/%.o: $(SRC_DIR)/kernel/%.c
	gcc $(CFLAGS) $(CDEBUG) -Wall -c -o $@ $< 

$(BUILD)/lib/%.o: $(SRC_DIR)/lib/%.c
	gcc $(CFLAGS) $(CDEBUG) -Wall -c -o $@ $< 

$(BUILD)/device/%.o: $(SRC_DIR)/device/%.c
	gcc $(CFLAGS) $(CDEBUG) -Wall -c -o $@ $< 

$(BUILD)/userprog/%.o: $(SRC_DIR)/userprog/%.c
	gcc $(CFLAGS) $(CDEBUG) -Wall -c -o $@ $< 

$(BUILD)/kernel.bin: $(ALL_TARGETS)
	ld $(LD_FLAGS) -o $@ $^

$(SYSTEM): $(BUILD)/kernel.bin
	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/mbr.bin $(BUILD)/boot/loader.bin \
	$(SYSTEM) \
	$(BUILD)/system.map
	yes | bximage -q -func=create -hd=10 -imgmode="flat" -sectsize=512 $@
	dd if=$(BUILD)/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=2 seek=2 conv=notrunc
	dd if=$(SYSTEM) of=$@ bs=512 count=200 seek=10 conv=notrunc


#虚拟机启动设置
.PHONY: bochs qemu qemug
bochs: $(BUILD)/master.img
	bochs

QEMU:= qemu-system-i386 -m 32M
QEMU+= -boot c
QEMU_DEBUG:= -s -S

QEMU_DISK:= -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw 

qemu: $(BUILD)/master.img
	$(QEMU) $(QEMU_DISK)

qemug: $(BUILD)/master.img
	$(QEMU) $(QEMU_DEBUG) $(QEMU_DISK) 

.PHONY: clear
clear: 
	rm -rf $(BUILD)/*
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/boot
	mkdir -p $(BUILD)/kernel
	mkdir -p $(BUILD)/lib
	mkdir -p $(BUILD)/device
	mkdir -p $(BUILD)/userprog