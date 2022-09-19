/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <context.h>
#include <gaia/elf.h>
#include <gaia/pmm.h>
#include <gaia/vmm.h>
#include <stdc-shim/string.h>
#include <stdint.h>

void elf_load(uint8_t *elf, uint64_t *entry, Context *context)
{
    Elf64Header *header = (Elf64Header *)elf;
    Elf64ProgramHeader *program_header = (Elf64ProgramHeader *)(elf + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++)
    {
        if (program_header->p_type == PT_LOAD)
        {
            size_t misalign = program_header->p_vaddr & (PAGE_SIZE - 1);
            size_t page_count = DIV_CEIL(misalign + program_header->p_memsz, PAGE_SIZE);

            for (size_t i = 0; i < page_count; i++)
            {
                uintptr_t addr = (uintptr_t)pmm_alloc_zero();

                vmm_mmap(context->space, PROT_READ | PROT_EXEC, MMAP_FIXED | MMAP_PHYS, (void *)program_header->p_vaddr, (void *)addr, PAGE_SIZE);
                memcpy((void *)(host_phys_to_virt(addr) + misalign), (void *)(elf + program_header->p_offset), program_header->p_filesz);
                memset((void *)(host_phys_to_virt(addr) + misalign + program_header->p_filesz), 0,
                       program_header->p_memsz - program_header->p_filesz);
            }
        }

        program_header = (Elf64ProgramHeader *)((uint8_t *)program_header + header->e_phentsize);
    }

    *entry = header->e_entry;
}
