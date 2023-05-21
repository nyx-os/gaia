/* SPDX-License-Identifier: BSD-2-Clause */
#include "kern/vm/vmem.h"
#include "machdep/vm.h"
#include <kern/vm/vm.h>
#include <kern/vm/vm_kernel.h>
#include <sys/queue.h>

vm_map_t vm_kmap;

void vm_init(charon_t charon)
{
    phys_init(charon);

    pmap_init();

    vmem_bootstrap();

    vm_kernel_init();
}

int vm_map(vm_map_t *map, vaddr_t *vaddrp, size_t size, vm_prot_t prot,
           vm_flags_t flags, vaddr_t *out)
{
    vaddr_t vaddr = 0;
    vm_map_entry_t *new_entry = kmalloc(sizeof(vm_map_entry_t));

    if (vaddrp) {
        vaddr = *vaddrp;
    } else {
        vaddr = (vaddr_t)vmem_alloc(&map->vmem, size, VM_INSTANTFIT);
    }

    new_entry->start = vaddr;
    new_entry->size = size;
    new_entry->prot = prot;

    LIST_INSERT_HEAD(&map->entries, new_entry, link);

    if (flags & VM_MAP_NOLAZY) {
        for (size_t i = 0; i < ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE; i++) {
            pmap_enter(map->pmap, vaddr + i * PAGE_SIZE, phys_allocz(), prot,
                       PMAP_USER);
        }
    }

    *out = vaddr;

    return 0;
}

int vm_fault(vm_map_t *map, vaddr_t vaddr)
{
    vm_map_entry_t *entry = NULL;

    // TODO: Handle pages that are already allocated
    LIST_FOREACH(entry, &map->entries, link)
    {
        if (vaddr >= entry->start && vaddr <= entry->start + entry->size) {
            pmap_enter(map->pmap, ALIGN_DOWN(vaddr, PAGE_SIZE), phys_allocz(),
                       entry->prot, PMAP_USER);

            return 0;
        }
    }

    return -1;
}

size_t vm_write(vm_map_t *map, vaddr_t addr, void *buf, size_t size)
{
    trace("vm_write(%p, %lx, %p, %ld)", (void *)map, addr, buf, size);

    for (size_t i = 0; i < ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE; i++) {
        assert(vm_fault(map, addr + i * PAGE_SIZE) == 0);
    }

    pmap_activate(map->pmap);

    memcpy((void *)addr, buf, size);

    pmap_activate(vm_kmap.pmap);

    return size;
}
