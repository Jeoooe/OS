ENTRY_POINT:=0xc0010000

BUILD:=./build
SRC_DIR:=./src

INCLUDE_DIR:=$(SRC_DIR)/include

LD_FLAGS:= -m elf_i386 -static -Ttext $(ENTRY_POINT)

CFLAGS:= -m32 -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector 
CFLAGS+= -I$(SRC_DIR)/include
CDEBUG:= -g


SYSTEM:=$(BUILD)/system.bin

ALL_TARGETS:= $(BUILD)/kernel/kernel.o $(BUILD)/lib/lib.o
ALL_TARGETS+= $(BUILD)/device/device.o $(BUILD)/userprog/userprog.o
ALL_TARGETS+= $(BUILD)/fs/filesys.o


all: $(BUILD)/master.img

$(BUILD)/boot/%.bin: $(SRC_DIR)/boot/%.asm
	nasm -f bin -o $@ $<

$(BUILD)/fs/filesys.o:
	(cd src/fs; make)

$(BUILD)/kernel/kernel.o:
	(cd src/kernel; make)

$(BUILD)/lib/lib.o:
	(cd src/lib; make)

$(BUILD)/device/device.o:
	(cd src/device; make)

$(BUILD)/userprog/userprog.o:
	(cd src/userprog; make)

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
QEMU_DISK+= -drive file=./slave.img,if=ide,index=1,media=disk,format=raw

qemu: $(BUILD)/master.img
	$(QEMU) $(QEMU_DISK)

qemug: $(BUILD)/master.img
	$(QEMU) $(QEMU_DEBUG) $(QEMU_DISK) 

.PHONY: slave
slave: 
	yes | bximage -q -func=create -hd=80 -imgmode="flat" -sectsize=512 slave.img
	sfdisk slave.img < slave.sfdisk

slave_clear:
	rm slave.img

.PHONY: clear
clear: 
	rm -rf $(BUILD)/*
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/boot
	mkdir -p $(BUILD)/kernel
	mkdir -p $(BUILD)/lib
	mkdir -p $(BUILD)/device
	mkdir -p $(BUILD)/userprog
	mkdir -p $(BUILD)/fs


dep:
	(cd src/kernel; make dep)
	(cd src/userprog; make dep)
	(cd src/fs; make dep)
	(cd src/lib; make dep)
	(cd src/device; make dep)