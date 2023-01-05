/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <context.h>
#include <gaia/elf.h>
#include <gaia/pmm.h>
#include <gaia/vm/vmm.h>
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

            VmCreateArgs args = {.addr = 0, .size = page_count * PAGE_SIZE, .flags = 0};
            VmObject obj = vm_create(args);

            VmMapArgs map_args = {.object = &obj, .protection = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC, .vaddr = program_header->p_vaddr, .flags = VM_MAP_FIXED};
            vm_map(context->space, map_args);
            vmm_write(context->space, program_header->p_vaddr, (void *)(elf + program_header->p_offset), program_header->p_filesz);
        }

        program_header = (Elf64ProgramHeader *)((uint8_t *)program_header + header->e_phentsize);
    }

    *entry = header->e_entry;
}
