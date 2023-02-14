/* SPDX-License-Identifier: BSD-2-Clause */
#include <machdep/vm.h>
#include <kern/vm/vm.h>
#include <kern/vm/phys.h>
#include <x86_64/limine.h>
#include <x86_64/cpuid.h>
#include <x86_64/asm.h>

#define PML_ENTRY(addr, offset) \
    (size_t)(addr & ((uintptr_t)0x1ff << offset)) >> offset;

#define PTE_PRESENT (1ull << 0ull)
#define PTE_WRITABLE (1ull << 1ull)
#define PTE_USER (1ull << 2ull)
#define PTE_NOT_EXECUTABLE (1ull << 63ull)
#define PTE_HUGE (1ull << 7ull)

#define PTE_ADDR_MASK ~(0xfff)
#define PTE_GET_ADDR(VALUE) ((VALUE)&PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

#define PTE_IS_PRESENT(x) (((x)&PTE_PRESENT) != 0)
#define PTE_IS_WRITABLE(x) (((x)&PTE_WRITABLE) != 0)
#define PTE_IS_HUGE(x) (((x)&PTE_HUGE) != 0)
#define PTE_IS_USER(x) (((x)&PTE_USER) != 0)
#define PTE_IS_NOT_EXECUTABLE(x) (((x)&PTE_NOT_EXECUTABLE) != 0)

static bool cpu_supports_1gb_pages = false;

extern char text_start_addr[], text_end_addr[];
extern char rodata_start_addr[], rodata_end_addr[];
extern char data_start_addr[], data_end_addr[];

static volatile struct limine_kernel_address_request kaddr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static uint64_t *get_next_level(uint64_t *table, size_t index, bool allocate)
{
    /* If the entry is already present, just return it. */
    if (PTE_IS_PRESENT(table[index])) {
        return (uint64_t *)P2V(PTE_GET_ADDR(table[index]));
    }

    /* If there's no entry in the page table, and we don't want to allocate, we abort. */
    if (!allocate) {
        return NULL;
    }

    /* Otherwise, we allocate a new entry */
    paddr_t new_table = phys_allocz();

    assert(new_table != 0);

    table[index] = new_table | PTE_PRESENT | PTE_USER | PTE_WRITABLE;

    return (uint64_t *)P2V(new_table);
}

static uint64_t vm_prot_to_mmu(vm_prot_t prot)
{
    uint64_t ret = 0;

    if (prot & VM_PROT_READ)
        ret |= PTE_PRESENT;

    if (prot & VM_PROT_WRITE)
        ret |= PTE_WRITABLE;

    if (!(prot & VM_PROT_EXECUTE))
        ret |= PTE_NOT_EXECUTABLE;

    return ret;
}

void pmap_init(void)
{
    size_t page_size = MIB(2);

    if (cpuid_has_exfeature(CPUID_EXFEATURE_PDPE1GB)) {
        cpu_supports_1gb_pages = true;
        page_size = GIB(1);
    }

    pmap_t kernel_pmap = pmap_create();

    vm_kmap.pmap = kernel_pmap;

    for (int i = 256; i < 512; i++) {
        assert(get_next_level((void *)P2V(kernel_pmap.pml4), i, true) != NULL);
    }

    uintptr_t text_start = ALIGN_DOWN((uintptr_t)text_start_addr, PAGE_SIZE);
    uintptr_t text_end = ALIGN_UP((uintptr_t)text_end_addr, PAGE_SIZE);
    uintptr_t rodata_start =
            ALIGN_DOWN((uintptr_t)rodata_start_addr, PAGE_SIZE);
    uintptr_t rodata_end = ALIGN_UP((uintptr_t)rodata_end_addr, PAGE_SIZE);
    uintptr_t data_start = ALIGN_DOWN((uintptr_t)data_start_addr, PAGE_SIZE);
    uintptr_t data_end = ALIGN_UP((uintptr_t)data_end_addr, PAGE_SIZE);

    struct limine_kernel_address_response *kaddr = kaddr_request.response;

    /* Text is readable and executable, writing is not allowed. */
    for (uintptr_t i = text_start; i < text_end; i += PAGE_SIZE) {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        pmap_enter(kernel_pmap, i, phys, VM_PROT_READ | VM_PROT_EXECUTE,
                   PMAP_NONE);
    }

    /* Read-only data is readable and NOT executable, writing is not allowed. */
    for (uintptr_t i = rodata_start; i < rodata_end; i += PAGE_SIZE) {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        pmap_enter(kernel_pmap, i, phys, VM_PROT_READ, PMAP_NONE);
    }

    /* Data is readable and writable, but is NOT executable. */
    for (uintptr_t i = data_start; i < data_end; i += PAGE_SIZE) {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;

        pmap_enter(kernel_pmap, i, phys, VM_PROT_READ | VM_PROT_WRITE,
                   PMAP_NONE);
    }

    /* Map the first 4GB+ of memory to the higher half. */
    for (size_t i = 0; i < MAX(GIB(4), phys_total_pages() * PAGE_SIZE);
         i += page_size) {
        pmap_enter(kernel_pmap, P2V(i), i,
                   VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
                   cpu_supports_1gb_pages ? PMAP_HUGE : PMAP_LARGE);
    }

    pmap_activate(kernel_pmap);
}

pmap_t pmap_create(void)
{
    pmap_t ret;

    ret.pml4 = phys_allocz();

    return ret;
}

void pmap_destroy(pmap_t pmap)
{
    /* TODO: destroy pmap properly */
    phys_free(pmap.pml4);
}

int pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
               pmap_flags_t flags)
{
    size_t level4 = PML_ENTRY(va, 39);
    size_t level3 = PML_ENTRY(va, 30);
    size_t level2 = PML_ENTRY(va, 21);
    size_t level1 = PML_ENTRY(va, 12);

    uint64_t *pml3 = get_next_level((void *)P2V(pmap.pml4), level4, true);

    /* If we're mapping 1G pages, we don't care about the rest of the mapping, only the pml3. */
    if (cpu_supports_1gb_pages && flags & PMAP_HUGE) {
        pml3[level3] = pa | vm_prot_to_mmu(prot) | PTE_HUGE;
        return 0;
    }

    /* If we're mapping 2M pages, we don't care about the rest of the mapping, only the pml2. */
    uint64_t *pml2 = get_next_level(pml3, level3, true);

    assert(pml2 != NULL);

    if (flags & PMAP_LARGE) {
        pml2[level2] = pa | vm_prot_to_mmu(prot) | PTE_HUGE;
        return 0;
    }

    uint64_t *pml1 = get_next_level(pml2, level2, true);

    pml1[level1] = pa | vm_prot_to_mmu(prot);

    return 0;
}

void pmap_remove(pmap_t pmap, vaddr_t va)
{
    size_t level4 = PML_ENTRY(va, 39);
    size_t level3 = PML_ENTRY(va, 30);
    size_t level2 = PML_ENTRY(va, 21);
    size_t level1 = PML_ENTRY(va, 12);

    uint64_t *pml3 = get_next_level((void *)P2V(pmap.pml4), level4, false);
    uint64_t *pml2 = get_next_level(pml3, level3, false);
    uint64_t *pml1 = get_next_level(pml2, level2, false);

    uint64_t *pml;
    size_t level = 0;

    if (PTE_IS_HUGE(PTE_GET_FLAGS(pml3[level3]))) {
        pml = pml3;
        level = level3;
    } else if (PTE_IS_HUGE(PTE_GET_FLAGS(pml2[level2]))) {
        pml = pml2;
        level = level2;
    } else {
        pml = pml1;
        level = level1;
    }

    phys_free(PTE_GET_ADDR(pml[level]));
    pml[level] = 0;

    pmap_invlpg(va);
}

void pmap_activate(pmap_t pmap)
{
    write_cr3(pmap.pml4);
}

void pmap_invlpg(vaddr_t va)
{
    asm volatile("invlpg %0" : : "m"(va) : "memory");
}
