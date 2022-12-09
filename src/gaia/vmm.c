#include "gaia/charon.h"
#include "gaia/debug.h"
#include "gaia/host.h"
#include <gaia/error.h>
#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/vmm.h>
#include <stdc-shim/string.h>

void vmm_space_init(VmmMapSpace *space)
{
    space->bump = MMAP_BUMP_BASE;
    space->mappings = NULL;
}

VmObject vm_create(VmCreateArgs args)
{
    VmObject ret = {0};

    ret.size = ALIGN_UP(args.size, 4096);

    if (args.flags & VM_MEM_DMA)
    {
        ret.buf = (void *)(ALIGN_DOWN(args.addr, 4096));
    }
    else
    {
        ret.buf = NULL;
    }

    return ret;
}

static bool is_unique_region_in_use(VmmMapSpace *space, uintptr_t address)
{
    VmMappableRegion *region = space->mappable_regions;

    while (region)
    {
        if (region->unique)
        {
            if (address >= region->address && address < region->address + region->size)
            {
                return true;
            }
        }
        region = region->next;
    }

    return false;
}

int vm_map_phys(VmmMapSpace *space, VmObject *object, uintptr_t phys, uintptr_t vaddr, uint16_t protection, uint16_t flags)
{
    uint16_t actual_prot = PAGE_USER;
    VmMapping *mapping = slab_alloc(sizeof(VmMapping));

    if (!(protection & VM_PROT_EXEC))
    {
        actual_prot |= PAGE_NOT_EXECUTABLE;
    }

    if (protection & VM_PROT_WRITE)
    {
        actual_prot |= PAGE_WRITABLE;
    }

    if (!(flags & VM_MAP_FIXED))
    {
        object->buf = (void *)space->bump;
        space->bump += object->size;
    }

    if (flags & VM_MAP_FIXED && vaddr != 0)
    {
        object->buf = (void *)(ALIGN_DOWN(vaddr, 4096));
    }

    assert(mapping != NULL);
    assert(space != NULL);

    mapping->actual_protection = actual_prot;
    mapping->object = *object;
    mapping->phys = (uintptr_t)phys;
    mapping->allocated_size = 0;
    mapping->address = (uintptr_t)object->buf;

    mapping->next = space->mappings;
    space->mappings = mapping;

    return 0;
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

    if (args.flags & VM_MAP_DMA)
    {
        phys = (uintptr_t)args.object->buf;

        if (is_unique_region_in_use(space, phys))
        {
            return ERR_IN_USE;
        }

        bool found = false;
        VmMappableRegion *region = space->mappable_regions;
        while (region)
        {
            if (phys >= region->address && phys < region->address + region->size)
            {
                found = true;
                break;
            }
            region = region->next;
        }

        if (!found)
        {
            return ERR_FORBIDDEN;
        }
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

    return ERR_SUCCESS;
}

void vm_new_mappable_region(VmmMapSpace *space, uintptr_t address, size_t size, uint16_t flags)
{
    VmMappableRegion *region = slab_alloc(sizeof(VmMappableRegion));
    region->address = address;
    region->size = size;
    region->unique = (flags & VM_REGION_UNIQUE);

    region->next = space->mappable_regions;
    space->mappable_regions = region;
}

bool vm_check_mappable_region(VmmMapSpace *space, uintptr_t address, size_t size)
{
    uintptr_t addr_end = address + size;
    Charon *charon = gaia_get_charon();
    CharonMemoryMap map = charon->memory_map;
    CharonModules modules = charon->modules;

    if (is_unique_region_in_use(space, address))
    {
        return false;
    }

    for (int i = 0; i < map.count; i++)
    {
        if (map.entries[i].type == MMAP_FREE || map.entries[i].type == MMAP_RECLAIMABLE)
            continue;

        if (map.entries[i].type == MMAP_FRAMEBUFFER || map.entries[i].type == MMAP_RESERVED)
        {
            // If address is in range of this region, return true
            if (address >= map.entries[i].base && addr_end <= map.entries[i].base + map.entries[i].size)
            {
                return true;
            }
        }
    }

    for (int i = 0; i < modules.count; i++)
    {
        if (address >= modules.modules[i].address && addr_end <= modules.modules[i].address + modules.modules[i].size)
        {
            return true;
        }
    }

    return false;
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

    while (mapping)
    {
        if (faulting_address >= mapping->address && faulting_address <= mapping->address + mapping->object.size && mapping->allocated_size != mapping->object.size)
        {
            found = true;

            uintptr_t virt = mapping->address + mapping->allocated_size;

            if (faulting_address > mapping->address)
            {
                virt = faulting_address;
            }

            else if (faulting_address > mapping->address + mapping->object.size)
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
            // trace("Demand paged one page at %p in range starting from %p (faulting_address=%p, phys=%p)", virt, mapping->address, faulting_address, (faulting_address - mapping->address) + mapping->phys);
#endif

            break;
        }

        mapping = mapping->next;
    }

    return found;
}

void vmm_write(VmmMapSpace *space, uintptr_t address, void *data, size_t count)
{
    for (size_t i = 0; i < ALIGN_UP(count, 4096) / 4096; i++)
    {
        assert(vmm_page_fault_handler(space, address + (i * 4096)) == true);
    }

    VmMapping *mapping = space->mappings;

    while (mapping != NULL)
    {
        if (address >= mapping->address && address <= mapping->address + mapping->object.size)
            break;

        mapping = mapping->next;
    }

    void *virt_addr = (void *)(host_phys_to_virt(mapping->phys) + (address - mapping->address));

    if (count % 8 == 0)
    {
        uint64_t *d = virt_addr;
        const uint64_t *s = data;

        for (size_t i = 0; i < count / 8; i++)
        {
            d[i] = s[i];
        }
    }
    else
    {
        uint8_t *d = virt_addr;
        const uint8_t *s = data;

        for (size_t i = 0; i < count; i++)
        {
            d[i] = s[i];
        }
    }
}
