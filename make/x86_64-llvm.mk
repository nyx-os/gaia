# Copyright (c) 2022, lg
# 
# SPDX-License-Identifier: BSD-2-Clause

CC = clang
LD = ld.lld
AS = nasm

ASFLAGS = -felf64

KERNEL_CFLAGS = $(BASE_CFLAGS) -target x86_64-unknown-elf -fno-stack-check \
        -fno-stack-protector \
        -fno-pic \
        -fno-pie \
        -mabi=sysv \
        -mno-80387 \
        -mno-mmx \
        -mno-3dnow \
        -mno-sse \
        -mno-sse2 \
        -mno-ssse3 \
        -mno-sse4 \
        -mno-sse4a \
        -mno-sse4.1 \
        -mno-sse4.2 \
        -mno-avx \
        -mno-avx2 \
        -mno-avx512f \
        -mno-red-zone \
        -msoft-float \
        -mcmodel=kernel

KERNEL_LINK_FLAGS = -Tsrc/gaia/arch/$(ARCH)/link.ld -nostdlib -melf_x86_64 -zmax-page-size=0x1000 -static