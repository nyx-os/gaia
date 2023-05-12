#include "kern/sched.h"
#include "kern/vm/vm.h"
#include "posix/posix.h"
#include <kern/term/term.h>
#include <kern/syscall.h>
#include <asm.h>

typedef uintptr_t sys_handler(syscall_frame_t frame);

static uintptr_t syscall_debug(syscall_frame_t frame)
{
    trace("debug(%s)", (char *)frame.param1);

    term_write((char *)frame.param1);
    term_write("\n");
    return 0;
}

static uintptr_t syscall_read(syscall_frame_t frame)
{
    trace("read(%ld, %p, %ld)", frame.param1, (void *)frame.param2,
          frame.param3);
    return sys_read(sched_curr()->parent, frame.param1, (void *)frame.param2,
                    frame.param3);
}

static uintptr_t syscall_open(syscall_frame_t frame)
{
    trace("open(%s, %ld)", (char *)frame.param1, frame.param2);

    return sys_open(sched_curr()->parent, (char *)frame.param1, frame.param2);
}

static uintptr_t syscall_close(syscall_frame_t frame)
{
    trace("close(%ld)", frame.param1);

    if (frame.param1 >= 3)
        return sys_close(sched_curr()->parent, frame.param1);
    return 0;
}

static uintptr_t syscall_write(syscall_frame_t frame)
{
    trace("write(%ld, %p, %ld)", frame.param1, (void *)frame.param2,
          frame.param3);

    if (frame.param1 < 3) {
        char *str = (char *)frame.param2;

        term_write(str);
        return frame.param3;
    }

    return sys_write(sched_curr()->parent, frame.param1, (void *)frame.param2,
                     frame.param3);
}

static uintptr_t syscall_seek(syscall_frame_t frame)
{
    return sys_seek(sched_curr()->parent, frame.param1, frame.param2,
                    frame.param3);
}

static uintptr_t syscall_tcb_set(syscall_frame_t frame)
{
    // TODO: move this somewhere else
    wrmsr(0xc0000100, (uint64_t)frame.param1);
    return 0;
}

static uintptr_t syscall_exit(syscall_frame_t frame)
{
    sched_curr()->state = STOPPED;
    SLIST_REMOVE(&sched_curr()->parent->threads, sched_curr(), thread,
                 task_link);

    log("thread %s exited with error code %ld", sched_curr()->name,
        frame.param1);

    sched_tick(frame.frame);

    return 0;
}

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20

static uintptr_t syscall_mmap(syscall_frame_t frame)
{
    struct {
        void *hint;
        size_t size;
        int prot;
        int flags;
        int fd;
        off_t offset;
        void **window;
    } *req = (void *)frame.param1;

    trace("mmap(%p, %lx, %d, %d, %d, %ld, %p)", req->hint, req->size, req->prot,
          req->flags, req->fd, req->offset, (void *)req->window);

    vm_prot_t prot = 0;
    size_t actual_flags = req->flags & 0xFFFFFFFF;

    if (req->prot & PROT_READ)
        prot |= VM_PROT_READ;
    if (req->prot & PROT_WRITE)
        prot |= VM_PROT_WRITE;
    if (req->prot & PROT_EXEC)
        prot |= VM_PROT_EXECUTE;

    if (actual_flags & MAP_FIXED) {
        vm_map(&sched_curr()->parent->map, (void *)&req->hint, req->size, prot,
               (void *)req->window);
    } else if (actual_flags & MAP_ANONYMOUS) {
        vm_map(&sched_curr()->parent->map, NULL, req->size, prot,
               (void *)req->window);
    }

    return 0;
}

static sys_handler *handlers[] = {
    [SYS_DEBUG] = syscall_debug,     [SYS_OPEN] = syscall_open,
    [SYS_CLOSE] = syscall_close,     [SYS_READ] = syscall_read,
    [SYS_WRITE] = syscall_write,     [SYS_SEEK] = syscall_seek,
    [SYS_MMAP] = syscall_mmap,       [SYS_EXIT] = syscall_exit,
    [SYS_TCB_SET] = syscall_tcb_set,
};

void syscall_handler(syscall_frame_t frame)
{
    *frame.ret = handlers[frame.num](frame);
}
