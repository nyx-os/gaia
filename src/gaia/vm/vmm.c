#include "gaia/charon.h"
#include "gaia/debug.h"
#include "gaia/firmware/acpi.h"
#include "gaia/host.h"
#include "gaia/vm/vmem.h"
#include <gaia/error.h>
#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/vm/vmm.h>
#include <stdc-shim/string.h>

void vmm_space_init(VmmMapSpace *space)
{
    space->mappings = NULL;
    space->phys_bindings = NULL;
    vmem_init(&space->vmem, "task space", (void *)USER_BASE, USER_SIZE, 0x1000, NULL, NULL, NULL, 0, 0);
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
        object->buf = vmem_alloc(&space->vmem, object->size, VM_INSTANTFIT);
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

    if (phys)
    {
        VmPhysBinding *binding = slab_alloc(sizeof(VmPhysBinding));
        binding->phys = phys;
        binding->virt = ALIGN_DOWN(mapping->address, 4096);
        binding->next = space->phys_bindings;
        space->phys_bindings = binding;
        mapping->is_dma = true;
    }

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
        args.object->buf = vmem_alloc(&space->vmem, args.object->size, VM_INSTANTFIT);
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

    if (phys)
    {
        VmPhysBinding *binding = slab_alloc(sizeof(VmPhysBinding));
        binding->phys = phys;
        binding->virt = ALIGN_DOWN(mapping->address, 4096);
        binding->next = space->phys_bindings;
        space->phys_bindings = binding;
        mapping->is_dma = true;
    }

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

    if (space->mappings == NULL)
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
                virt = ALIGN_DOWN(faulting_address, 4096);
            }

            else if (faulting_address > mapping->address + mapping->object.size)
            {
                found = false;
                break;
            }

            if (!mapping->is_dma)
            {
                void *phys = pmm_alloc_zero();
                VmPhysBinding *binding = slab_alloc(sizeof(VmPhysBinding));

                binding->phys = (uintptr_t)phys;
                binding->virt = virt;
                binding->next = space->phys_bindings;
                space->phys_bindings = binding;
                mapping->phys = (uintptr_t)phys;
                host_map_page(space->pagemap, virt, (uintptr_t)phys, mapping->actual_protection);
            }
            else
            {
                host_map_page(space->pagemap, virt, mapping->phys + (virt - mapping->address), mapping->actual_protection);
            }

            mapping->allocated_size += 4096;

#ifdef DEBUG
            // trace("Demand paged one page at %p in range starting from %p (faulting_address=%p, phys=%p)", virt, mapping->address, faulting_address, mapping->phys + (virt - mapping->address));
#endif

            break;
        }

        mapping = mapping->next;
    }

    return found;
}

void vmm_write(VmmMapSpace *space, uintptr_t address, void *data, size_t count)
{
    size_t bytes_remaining = count;
    size_t bytes_wrote = 0;

    while (bytes_remaining != 0)
    {
        uintptr_t aligned_address = ALIGN_DOWN(address + bytes_wrote, 4096);
        size_t bytes_to_write = MIN(bytes_remaining, 4096);

        assert(vmm_page_fault_handler(space, aligned_address) == true);

        VmPhysBinding *binding = space->phys_bindings;

        while (binding)
        {
            if (aligned_address >= binding->virt && aligned_address < binding->virt + PAGE_SIZE)
            {
                break;
            }
            binding = binding->next;
        }
        if (!binding)
        {
            panic("address %p is not allocated", aligned_address);
        }

        // If writing 4096 bytes will result in out of bounds, write only the bytes until the end of the page
        size_t offset = (address + bytes_wrote) - binding->virt;

        if (bytes_to_write + offset > PAGE_SIZE)
        {
            bytes_to_write = PAGE_SIZE - offset;
        }

        void *virt_addr = (void *)host_phys_to_virt(binding->phys + (offset));

        host_accelerated_copy(virt_addr, (uint8_t *)data + bytes_wrote, bytes_to_write);

        bytes_remaining -= bytes_to_write;
        bytes_wrote += bytes_to_write;
    }
}
