# Copyright (c) 2022, lg
# 
# SPDX-License-Identifier: BSD-2-Clause

-include make/config.mk

KERNEL_C_SOURCES = 	 $(wildcard src/gaia/*.c) \
				 	 $(wildcard src/gaia/*/*.c) \
				 	 $(wildcard src/gaia/arch/$(ARCH)/*.c) \
				 	 $(wildcard src/lib/gaia/*.c) \
				 	 $(wildcard src/lib/stdc-shim/*.c)

KERNEL_ASM_SOURCES = $(wildcard src/gaia/arch/$(ARCH)/*.s)


KERNEL_C_OBJS = $(patsubst %.c, $(BUILDDIR)/%.c.o, $(KERNEL_C_SOURCES))
KERNEL_OBJS = $(KERNEL_C_OBJS) \
			  $(patsubst %.s, $(BUILDDIR)/%.s.o, $(KERNEL_ASM_SOURCES))

.DEFAULT_GOAL: all


ifeq (,$(wildcard ./.config))
.PHONY: all
all:
	@echo "Looks like Gaia is not configured."
	@echo "Run 'make menuconfig' to configure it."
	@exit 1
endif

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJS)
	$(LD) -o $@ $^ $(KERNEL_LINK_FLAGS)

$(BUILDDIR)/%.c.o: %.c
	@$(DIRECTORY_GUARD)
	$(CC) $(KERNEL_CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.s.o: %.s
	@$(DIRECTORY_GUARD)
	$(AS) $(ASFLAGS) $< -o $@

run: 
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -enable-kvm -no-reboot -no-shutdown -cpu host -cdrom $(ISO)

debug:
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -d int -no-reboot -no-shutdown -cdrom $(ISO)

tcg:
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -cdrom $(ISO)

gdb:
	qemu-system-$(ARCH) -m $(QEMU_MEMORY) -M q35 -no-reboot -debugcon stdio -enable-kvm -cdrom $(ISO) -s -S

clean:
	-rm -rf $(BUILDDIR)

menuconfig:
	./scripts/menuconfig

.PHONY: all run clean menuconfig

DEPS += $(KERNEL_C_OBJS:.c.o=.c.d)
-include $(DEPS)
