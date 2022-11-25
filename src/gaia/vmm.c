#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/vmm.h>
#include <stdc-shim/string.h>

void vmm_space_init(VmmMapSpace *space)
{
    space->bump = MMAP_BUMP_BASE;
}

VmObject vm_create(VmCreateArgs args)
{
    VmObject ret = {0};

    ret.size = ALIGN_UP(args.size, 4096);

    if (args.flags & VM_MEM_DMA)
    {
        ret.buf = (void *)ALIGN_DOWN((uintptr_t)args.addr, 4096);
    }
    else
    {
        ret.buf = NULL;
    }

    return ret;
}

int vm_map(VmmMapSpace *space, VmMapArgs args)
{
    uint16_t actual_prot = PAGE_USER;
    VmMapping *mapping = slab_alloc(sizeof(VmMapping));
    uintptr_t phys = 0;

    if (!(args.protection & VM_PROT_EXEC))
    {
        actual_prot |= PAGE_NOT_EXECUTABLE;
    }

    if (args.protection & VM_PROT_WRITE)
    {
        actual_prot |= PAGE_WRITABLE;
    }

    if (args.flags & VM_MAP_PHYS)
    {
        phys = (uintptr_t)args.object->buf;
    }

    if (!(args.flags & VM_MAP_FIXED))
    {

        args.object->buf = (void *)space->bump;
        space->bump += args.object->size;
    }

    if (args.flags & VM_MAP_FIXED && args.vaddr != 0)
    {
        args.object->buf = (void *)(ALIGN_DOWN(args.vaddr, 4096));
    }

    assert(mapping != NULL);
    assert(space != NULL);

    mapping->actual_protection = actual_prot;
    mapping->object = *args.object;
    mapping->phys = (uintptr_t)phys;
    mapping->allocated_size = 0;
    mapping->address = (uintptr_t)args.object->buf;

    mapping->next = space->mappings;
    space->mappings = mapping;

    return 0;
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

    VmMapping *mapping = space->mappings;

    uintptr_t aligned_addr = ALIGN_DOWN(faulting_address, 4096);

    while (mapping)
    {
        if (aligned_addr >= mapping->address && aligned_addr <= mapping->address + mapping->object.size && mapping->allocated_size != mapping->object.size)
        {
            found = true;

            uintptr_t virt = mapping->address + mapping->allocated_size;

            if (aligned_addr > mapping->address)
            {
                virt = aligned_addr;
            }

            else if (aligned_addr > mapping->address + mapping->object.size)
            {
                found = false;
                break;
            }

            if (!mapping->phys)
            {
                void *phys = pmm_alloc_zero();
                host_map_page(space->pagemap, virt, (uintptr_t)phys, mapping->actual_protection);
                mapping->phys = (uintptr_t)phys;
            }

            else
            {
                host_map_page(space->pagemap, virt, (uintptr_t)mapping->phys + mapping->allocated_size, mapping->actual_protection);
            }

            mapping->allocated_size += 4096;

#ifdef DEBUG
             //trace("Demand paged one page at %p in range starting from %p (aligned_addr=%p)", virt, mapping->address, aligned_addr);
#endif

            break;
        }

        mapping = mapping->next;
    }

    return found;
}

void vmm_write(VmmMapSpace *space, uintptr_t address, void *data, size_t count)
{
    assert(count < 4096);

    assert(vmm_page_fault_handler(space, address) == true);

    VmMapping *mapping = space->mappings;

    while (mapping != NULL)
    {
        if (address >= mapping->address && address <= mapping->address + mapping->object.size)
            break;

        mapping = mapping->next;
    }

    void *virt_addr = (void *)(host_phys_to_virt(mapping->phys) + address - mapping->address);
    memcpy(virt_addr, data, count);
}
