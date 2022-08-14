# Copyright (c) 2022, lg
# 
# SPDX-License-Identifier: BSD-2-Clause

-include make/config.mk
-include $(DEPS)

KERNEL_C_SOURCES = 	 $(wildcard src/gaia/*.c) \
				 	 $(wildcard src/gaia/*/*.c) \
				 	 $(wildcard src/gaia/arch/$(ARCH)/*.c) \
				 	 $(wildcard src/lib/gaia/*.c) \
				 	 $(wildcard src/lib/stdc-shim/*.c) \
				
KERNEL_ASM_SOURCES = $(wildcard src/gaia/arch/$(ARCH)/*.asm)


KERNEL_C_OBJS = $(patsubst %.c, $(BUILDDIR)/%.c.o, $(KERNEL_C_SOURCES))
KERNEL_OBJS = $(KERNEL_C_OBJS) \
			  $(patsubst %.asm, $(BUILDDIR)/%.asm.o, $(KERNEL_ASM_SOURCES))

all: $(ISO)

$(ISO): $(KERNEL)
	./scripts/make-image.sh $(BUILDDIR)

$(KERNEL): $(KERNEL_OBJS)
	$(LD) -o $@ $^ $(KERNEL_LINK_FLAGS)

$(BUILDDIR)/%.c.o: %.c
	@$(DIRECTORY_GUARD)
	$(CC) $(KERNEL_CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.asm.o: %.asm
	@$(DIRECTORY_GUARD)
	$(AS) $(ASFLAGS) $< -o $@

run: $(ISO)
	qemu-system-$(ARCH) -m 2G -no-shutdown -no-reboot -debugcon stdio -enable-kvm -cdrom $^

clean:
	-rm -rf $(BUILDDIR)

.PHONY: all run clean