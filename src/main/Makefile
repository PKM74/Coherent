CC=gcc -fpermissive # -fpermissive is NEEDED, otherwise will not build due to error additons from C99 standard
ASM=nasm
BUILD_DIR=../../build/main

.PHONY: all bin conf etc lib usr kernel PS2_KERNEL 286_KERNEL clean

# Automaticly Builds normal Kernel
all: bin conf etc lib kernel #usr
# Build for 286 kernel (Untested)
286: bin conf etc lib usr 286_KERNEL
# Build for PS2 kernel (Untested)
PS2: bin conf etc lib usr PS2_KERNEL

bin:
	$(MAKE) -C bin/ BUILD_DIR=$(abspath $(BUILD_DIR)/bin)
conf:
	$(MAKE) -C conf/ BUILD_DIR=$(abspath $(BUILD_DIR)/conf)
etc:
	$(MAKE) -C etc/ BUILD_DIR=$(abspath $(BUILD_DIR)/etc)
lib:
	$(MAKE) -C lib/ BUILD_DIR=$(abspath $(BUILD_DIR)/lib)

#usr:
#	$(MAKE) -C usr/ BUILD_DIR=$(abspath $(BUILD_DIR)/usr)

kernel:
	$(MAKE) -C kernel/ BUILD_DIR=$(abspath $(BUILD_DIR)/kernel)