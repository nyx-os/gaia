#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/vmm.h>

void vmm_space_init(VmmMapSpace *space)
{
    space->ranges_head = NULL;
}

void *vmm_mmap(VmmMapSpace *space, uint16_t prot, uint16_t flags, void *addr, void *phys, size_t size)
{
    void *ret = NULL;
    uint16_t actual_prot = PAGE_USER;

    if (prot & PROT_NONE)
    {
        actual_prot = PAGE_USER | PAGE_NONE;
    }

    if (!(prot & PROT_EXEC))
    {
        actual_prot |= PAGE_NOT_EXECUTABLE;
    }

    if (prot & PROT_WRITE)
    {
        actual_prot |= PAGE_WRITABLE;
    }

    if (flags & MMAP_FIXED)
    {
        ret = (void *)ALIGN_DOWN((uintptr_t)addr, 4096);
    }

    if (flags & MMAP_ANONYMOUS)
    {
        ret = (void *)space->bump;
        space->bump += size;
    }

    VmmMapRange *new_range = slab_alloc(sizeof(VmmMapRange));
    new_range->allocated_size = 0;
    new_range->address = (uintptr_t)ret;
    new_range->phys = (uintptr_t)phys;
    new_range->size = size;
    new_range->flags = flags;
    new_range->prot = actual_prot;

    new_range->next = space->ranges_head;
    space->ranges_head = new_range;

    return ret;
}

void vmm_munmap(VmmMapSpace *space, uintptr_t addr)
{
    warn("munmap is not implemented");
    DISCARD(space);
    DISCARD(addr);
}

bool vmm_page_fault_handler(VmmMapSpace *space, uintptr_t faulting_address)
{
    bool found = false;
    if (space == NULL || faulting_address == 0)
        return false;

    VmmMapRange *range = space->ranges_head;

    while (range)
    {
        if (ALIGN_DOWN(faulting_address, 4096) >= range->address && range->allocated_size != range->size)
        {
            found = true;

            if (!(range->flags & MMAP_PHYS))
            {
                void *phys = pmm_alloc_zero();
                host_map_page(space->pagemap, range->address + range->allocated_size, (uintptr_t)phys, range->prot);
            }

            else if (range->flags & MMAP_PHYS)
            {

                host_map_page(space->pagemap, range->address + range->allocated_size, (uintptr_t)range->phys, range->prot);
            }

            range->allocated_size += 4096;

#ifdef DEBUG
            trace("Demand paged one page from %p", faulting_address);
#endif

            break;
        }

        range = range->next;
    }

    return found;
}
