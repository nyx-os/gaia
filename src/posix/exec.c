#include "asm.h"
#include "kern/sched.h"
#include "kern/vm/vm_kernel.h"
#include "machdep/cpu.h"
#include <posix/posix.h>
#include <elf.h>
#include <kern/vm/vm.h>

typedef struct {
    uintptr_t at_entry;
    uintptr_t at_phdr;
    uintptr_t at_phent;
    uintptr_t at_phnum;
} auxval_t;

static uintptr_t load_elf(task_t *task, vnode_t *file, auxval_t *auxval,
                          uintptr_t base, char *ld_path)
{
    Elf64_Ehdr header = { 0 };

    VOP_READ(file, &header, sizeof(Elf64_Ehdr), 0);

    if (header.e_ident[0] != ELFMAG0 || header.e_ident[1] != ELFMAG1 ||
        header.e_ident[2] != ELFMAG2 || header.e_ident[3] != ELFMAG3) {
        return -1;
    }

    auxval->at_phdr = 0;
    auxval->at_phent = header.e_phentsize;
    auxval->at_phnum = header.e_phnum;

    for (int i = 0; i < header.e_phnum; i++) {
        Elf64_Phdr phdr;

        VOP_READ(file, &phdr, header.e_phentsize,
                 header.e_phoff + i * header.e_phentsize);

        void *buf = kmalloc(phdr.p_filesz);

        VOP_READ(file, buf, phdr.p_filesz, phdr.p_offset);

        switch (phdr.p_type) {
        case PT_LOAD: {
            size_t misalign = phdr.p_vaddr & (4096 - 1);
            size_t page_count = DIV_CEIL(misalign + phdr.p_memsz, 4096);

            vaddr_t addr = base + phdr.p_vaddr;

            vm_map(&task->map, &addr, page_count * PAGE_SIZE,
                   VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
                   VM_MAP_NOLAZY, &addr);

            pmap_t prev_pmap = { .pml4 = read_cr3() };

            pmap_activate(task->map.pmap);

            memcpy((void *)addr, buf, phdr.p_filesz);

            pmap_activate(prev_pmap);

            break;
        }
        case PT_PHDR:
            auxval->at_phdr = base + phdr.p_vaddr;
            break;

        case PT_INTERP: {
            if (!ld_path)
                break;

            memcpy(ld_path, buf, phdr.p_filesz);

            (ld_path)[phdr.p_filesz] = 0;
        }

            kfree(buf, phdr.p_filesz);
        }
    }

    auxval->at_entry = base + header.e_entry;
    return 0;
}

int sys_execve(task_t *proc, const char *path, char const *argv[],
               char const *envp[])
{
    vnode_t *vn;
    vattr_t attr;

    int r = vfs_find_and(proc->cwd, &vn, path, NULL, VFS_FIND_OR_ERROR, &attr);
    uintptr_t stack_top = USER_STACK_TOP;
    size_t *stack_ptr = (size_t *)stack_top;
    auxval_t auxval;
    char ld_path[255] = { 0 };
    size_t nargs = 0, nenv = 0;
    uintptr_t entry = 0;

    if (r < 0) {
        panic("Error: %d", r);
        return r;
    }

    load_elf(proc, vn, &auxval, 0, (char *)ld_path);

    entry = auxval.at_entry;

    for (char **elem = (char **)argv; *elem; elem++) {
        stack_ptr = (void *)((uint8_t *)stack_ptr - (strlen(*elem) + 1));
        nargs++;
    }

    for (char **elem = (char **)envp; *elem; elem++) {
        stack_ptr = (void *)((uint8_t *)stack_ptr - (strlen(*elem) + 1));
        nenv++;
    }

    stack_ptr = (void *)((uint8_t *)stack_ptr - ((uintptr_t)stack_ptr & 0xf));
    if ((nargs + nenv + 1) & 1)
        stack_ptr--;

    --stack_ptr;
    --stack_ptr;

    stack_ptr -= 2;
    stack_ptr -= 2;
    stack_ptr -= 2;
    stack_ptr -= 2;

    stack_ptr--;
    stack_ptr -= nenv;
    stack_ptr--;
    stack_ptr -= nargs;
    stack_ptr--;

    uintptr_t required_size = (stack_top - (uintptr_t)stack_ptr);

    // We don't mess up allocation metadata this way
    size_t *stack = vm_kernel_alloc(
            ALIGN_UP(required_size, PAGE_SIZE) / PAGE_SIZE, false);

    for (char **elem = (char **)envp; *elem; elem++) {
        stack = (void *)((uint8_t *)stack - (strlen(*elem) + 1));
        strncpy((char *)stack, *elem, strlen(*elem));
    }

    for (char **elem = (char **)argv; *elem; elem++) {
        stack = (void *)((uint8_t *)stack - (strlen(*elem) + 1));
        strncpy((char *)stack, *elem, strlen(*elem));
    }

    /* Align strp to 16-byte so that the following calculation becomes easier. */
    stack = (void *)((uint8_t *)stack - ((uintptr_t)stack & 0xf));

    /* Make sure the *final* stack pointer is 16-byte aligned.
            - The auxv takes a multiple of 16-bytes; ignore that.
            - There are 2 markers that each take 8-byte; ignore that, too.
            - Then, there is argc and (nargs + nenv)-many pointers to args/environ.
              Those are what we *really* care about. */
    if ((nargs + nenv + 1) & 1)
        stack--;

    *(--stack) = 0;
    *(--stack) = 0; /* Zero auxiliary vector entry */
    stack -= 2;
    *stack = AT_ENTRY;
    *(stack + 1) = auxval.at_entry;
    stack -= 2;
    *stack = AT_PHDR;
    *(stack + 1) = auxval.at_phdr;
    stack -= 2;
    *stack = AT_PHENT;
    *(stack + 1) = auxval.at_phent;
    stack -= 2;
    *stack = AT_PHNUM;
    *(stack + 1) = auxval.at_phnum;

    uintptr_t sa = USER_STACK_TOP;

    *(--stack) = 0; /* Marker for end of environ */
    stack -= nenv;
    for (size_t i = 0; i < nenv; i++) {
        sa -= strlen(envp[i]) + 1;
        stack[i] = sa;
    }

    *(--stack) = 0; /* Marker for end of argv */

    stack -= nargs;
    for (size_t i = 0; i < nargs; i++) {
        sa -= strlen(argv[i]) + 1;
        stack[i] = sa;
    }

    *(--stack) = nargs; /* argc */

    vaddr_t addr = USER_STACK_TOP - required_size;

    vm_map(&proc->map, &addr, ALIGN_UP(required_size, PAGE_SIZE),
           VM_PROT_READ | VM_PROT_WRITE, VM_MAP_NOLAZY, &addr);

    pmap_t prev_pmap = { .pml4 = read_cr3() };
    pmap_activate(proc->map.pmap);

    memcpy((void *)(USER_STACK_TOP - required_size), stack, required_size);

    pmap_activate(prev_pmap);

    addr = (USER_STACK_TOP - required_size) - USER_STACK_SIZE;

    vm_map(&proc->map, &addr, USER_STACK_SIZE, VM_PROT_READ | VM_PROT_WRITE, 0,
           &addr);

    if (ld_path[0]) {
        log("LD PATH: %s", ld_path);

        vnode_t *ldn;
        vattr_t ld_attr = { 0 };

        r = vfs_find_and(NULL, &ldn, ld_path, NULL, VFS_FIND_OR_ERROR,
                         &ld_attr);

        if (r < 0) {
            panic("error finding ld path");
        }

        auxval_t ld_aux = { 0 };
        r = load_elf(proc, ldn, &ld_aux, 0x40000000, NULL);

        entry = ld_aux.at_entry;

        if (r != 0) {
            panic("error: %d", r);
        }
    }

    thread_t *thread;

    SLIST_FOREACH(thread, &proc->threads, task_link)
    {
        kfree(thread, sizeof(*thread));
    }

    SLIST_INIT(&proc->threads);

    sched_new_thread(
            path, proc,
            cpu_new_context(entry, USER_STACK_TOP - required_size, true), true);

    return 0;
}
