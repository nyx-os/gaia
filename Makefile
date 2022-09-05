# Copyright (c) 2022, lg
# 
# SPDX-License-Identifier: BSD-2-Clause

-include make/config.mk

KERNEL_C_SOURCES = 	 $(wildcard src/gaia/*.c) \
				 	 $(wildcard src/gaia/*/*.c) \
				 	 $(wildcard src/gaia/arch/$(ARCH)/*.c) \
				 	 $(wildcard src/lib/gaia/*.c) \
				 	 $(wildcard src/lib/stdc-shim/*.c) \
				
KERNEL_ASM_SOURCES = $(wildcard src/gaia/arch/$(ARCH)/*.s)


KERNEL_C_OBJS = $(patsubst %.c, $(BUILDDIR)/%.c.o, $(KERNEL_C_SOURCES))
KERNEL_OBJS = $(KERNEL_C_OBJS) \
			  $(patsubst %.s, $(BUILDDIR)/%.s.o, $(KERNEL_ASM_SOURCES))

all: $(ISO)

servers:
	$(MAKE) -C thirdparty/olympus
	cp thirdparty/olympus/build/*.elf sysroot

$(ISO): $(KERNEL) servers
	./scripts/make-image.sh $(BUILDDIR)

$(KERNEL): $(KERNEL_OBJS)
	$(LD) -o $@ $^ $(KERNEL_LINK_FLAGS)

$(BUILDDIR)/%.c.o: %.c
	@$(DIRECTORY_GUARD)
	$(CC) $(KERNEL_CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.s.o: %.s
	@$(DIRECTORY_GUARD)
	$(AS) $(ASFLAGS) $< -o $@

run: $(ISO)
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -enable-kvm -cpu host -cdrom $^

debug: $(ISO)
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -d int -cdrom $^

tcg: $(ISO)
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -cdrom $^

gdb: $(ISO)
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -enable-kvm -cdrom $^ -s -S

clean:
	-rm -rf $(BUILDDIR)

menuconfig:
	./scripts/menuconfig

.PHONY: all run clean menuconfig

DEPS += $(KERNEL_C_OBJS:.c.o=.c.d)
-include $(DEPS)
