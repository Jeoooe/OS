BUILD:=./build
SRC_DIR:=./src

SOURCES:= $(SRC_DIR)/mbr.asm

BOOT_ASM:= $(SRC_DIR)/boot/loader.asm

$(BUILD)/master.img: $(BUILD)/boot/mbr.bin $(BUILD)/boot/loader.bin 
	yes | bximage -q -func=create -hd=10 -imgmode="flat" -sectsize=512 $@
	dd if=$< of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=1 seek=2 conv=notrunc

$(BUILD)/boot/%.bin: $(SRC_DIR)/boot/%.asm
	mkdir -p $(BUILD)/boot
	nasm -f bin -o $@ $<


.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs

.PHONY: clear
clear: 
	rm -rf $(BUILD)/*
	mkdir -p $(BUILD)