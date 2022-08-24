# Copyright (c) 2022, lg
# 
# SPDX-License-Identifier: BSD-2-Clause

-include .config

DIRECTORY_GUARD = mkdir -p $(@D)
BASE_CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Isrc/gaia/arch/$(ARCH) -Isrc -Isrc/lib -Ithirdparty/limine -MMD -fno-builtin
ISO = nyx.iso
BUILDDIR = build
KERNEL = $(BUILDDIR)/kernel.elf
QEMU_MEMORY = 2G

ifeq ($(CONFIG_LLVM), y)
	TOOLCHAIN = llvm
endif

ifeq ($(CONFIG_X86_64), y)
	ARCH = x86_64
endif

ifeq ($(CONFIG_DEBUG), y)
	BASE_CFLAGS += -O0 -ggdb -fsanitize=undefined -DDEBUG
endif

ifeq ($(CONFIG_RELEASE), y)
	BASE_CFLAGS += -O2
endif

-include make/$(ARCH)-$(TOOLCHAIN).mk