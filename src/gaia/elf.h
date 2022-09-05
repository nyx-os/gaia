/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_ELF_H
#define SRC_GAIA_ELF_H
#include <gaia/base.h>
#include <gaia/sched.h>

typedef struct PACKED
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64Header;

typedef struct PACKED
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64ProgramHeader;

#define PT_LOAD 0x00000001
#define PT_INTERP 0x00000003
#define PT_PHDR 0x00000006

void elf_load(uint8_t *elf, uint64_t *entry_point, Context *context);

#endif /* SRC_GAIA_ELF_H */
