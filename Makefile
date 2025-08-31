BUILD:=./build
SRC_DIR:=./src

SOURCES:= $(SRC_DIR)/mbr.asm

$(BUILD)/master.img: $(BUILD)/mbr.bin
	yes | bximage -q -func=create -hd=10 -imgmode="flat" -sectsize=512 $@
	dd if=$< of=$@ bs=512 count=1 conv=notrunc

$(BUILD)/mbr.bin: $(SRC_DIR)/mbr.asm
	mkdir -p $(BUILD)
	nasm -f bin -o $@ $<


.PHONY: bochs
bochs: $(BUILD)/master.img
	bochs

.PHONY: clear
clear: 
	rm -rf $(BUILD)/*
	mkdir -p $(BUILD)