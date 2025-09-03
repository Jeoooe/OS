BUILD:=./build
SRC_DIR:=./src

SOURCES:= $(SRC_DIR)/mbr.asm

BOOT_ASM:= $(SRC_DIR)/boot/loader.asm

.PHONY: default
default: $(BUILD)/master.img

$(BUILD)/boot/%.bin: $(SRC_DIR)/boot/%.asm
	nasm -f bin -o $@ $<

$(BUILD)/kernel/%.o: $(SRC_DIR)/kernel/%.c
	gcc -m32 -c -o $@ $< 

$(BUILD)/kernel.bin: $(BUILD)/kernel/main.o
	ld -m elf_i386 -Ttext 0xc0001500 -e kernel_main -o $@ $^


$(BUILD)/master.img: \
	$(BUILD)/boot/mbr.bin $(BUILD)/boot/loader.bin \
	$(BUILD)/kernel.bin
	yes | bximage -q -func=create -hd=10 -imgmode="flat" -sectsize=512 $@
	dd if=$(BUILD)/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=1 seek=2 conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 count=200 seek=9 conv=notrunc


.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs

.PHONY: clear
clear: 
	rm -rf $(BUILD)/*
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/boot
	mkdir -p $(BUILD)/kernel