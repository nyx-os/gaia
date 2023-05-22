#include "kern/sched.h"
#include "kern/vm/vm.h"
#include "machdep/cpu.h"
#include "machdep/intr.h"
#include "posix/posix.h"
#include <kern/term/term.h>
#include <kern/syscall.h>
#include <errno.h>
#include <asm.h>
#include <sys/queue.h>

typedef uintptr_t sys_handler(syscall_frame_t frame);

static uintptr_t syscall_debug(syscall_frame_t frame)
{
    (void)frame;
    debug("%s", (char *)frame.param1);

    return 0;
}

static uintptr_t syscall_read(syscall_frame_t frame)
{
    uintptr_t ret = sys_read(sched_curr()->parent, frame.param1,
                             (void *)frame.param2, frame.param3);
    trace("read(%ld, %p, %ld) => %ld", frame.param1, (void *)frame.param2,
          frame.param3, ret);
    return ret;
}

static uintptr_t syscall_open(syscall_frame_t frame)
{
    uintptr_t ret =
            sys_open(sched_curr()->parent, (char *)frame.param1, frame.param2);

    trace("open(%s, %ld) => %ld", (char *)frame.param1, frame.param2, ret);
    return ret;
}

static uintptr_t syscall_close(syscall_frame_t frame)
{
    trace("close(%ld)", frame.param1);

    return sys_close(sched_curr()->parent, frame.param1);
}

static uintptr_t syscall_getpid(syscall_frame_t frame)
{
    (void)frame;
    trace("getpid()");
    return sched_curr()->parent->pid;
}

static uintptr_t syscall_getppid(syscall_frame_t frame)
{
    (void)frame;
    trace("getppid()");
    return sched_curr()->parent->ppid;
}

static uintptr_t syscall_write(syscall_frame_t frame)
{
    size_t ret = sys_write(sched_curr()->parent, frame.param1,
                           (void *)frame.param2, frame.param3);

    trace("write(%ld, %p, %ld) => %ld", frame.param1, (void *)frame.param2,
          frame.param3, ret);
    return ret;
}

static uintptr_t syscall_seek(syscall_frame_t frame)
{
    trace("seek(%ld, %ld, %ld)", frame.param1, frame.param2, frame.param3);

    return sys_seek(sched_curr()->parent, frame.param1, frame.param2,
                    frame.param3);
}

static uintptr_t syscall_stat(syscall_frame_t frame)
{
    trace("stat(%ld, %s, %ld, %p)", frame.param1, (char *)frame.param2,
          frame.param3, (void *)frame.param4);

    return sys_stat(sched_curr()->parent, frame.param1, (char *)frame.param2,
                    frame.param3, (void *)frame.param4);
}

static uintptr_t syscall_readdir(syscall_frame_t frame)
{
    trace("readdir(%ld, %p, %ld, %p)", frame.param1, (void *)frame.param2,
          frame.param3, (void *)frame.param4);

    return sys_readdir(sched_curr()->parent, frame.param1, (void *)frame.param2,
                       frame.param3, (size_t *)frame.param4);
}

static uintptr_t syscall_tcb_set(syscall_frame_t frame)
{
    trace("tcb_set()");
    // TODO: move this somewhere else
    wrmsr(0xc0000100, (uint64_t)frame.param1);
    return 0;
}

void sched_add_back(thread_t *thread, intr_frame_t *frame);

static uintptr_t syscall_exit(syscall_frame_t frame)
{
    sched_curr()->state = STOPPED;
    SLIST_REMOVE(&sched_curr()->parent->threads, sched_curr(), thread,
                 task_link);

    if (sched_curr()->parent->parent) {
        sched_curr()->parent->parent->children_n--;

        if (sched_curr()->parent->parent->state == WAITING_FOR_CHILD) {
            sched_curr()->parent->parent->state = RUNNING;
            sched_curr()->parent->parent->child_exited = true;
            sched_curr()->parent->parent->ctx.regs.rax =
                    sched_curr()->parent->pid;
            sched_curr()->parent->parent->child_status = frame.param1;
            sched_curr()->parent->parent->child_that_exited =
                    sched_curr()->parent->pid;

            sched_curr()->parent->stopped = true;

            log("I exited %s, %p", sched_curr()->parent->parent->name,
                (void *)sched_curr()->parent->parent);

            sched_add_back(sched_curr()->parent->parent, frame.frame);
            sched_dump();
        }
    }

    log("thread %s (in process %d) exited with error code %ld",
        sched_curr()->name, sched_curr()->parent->pid, frame.param1);

    sched_tick(frame.frame);

    // We want to restore the frame's rax so that stuff like e.g fork() doesn't fail
    return frame.frame->rax;
}

static uintptr_t syscall_fork(syscall_frame_t frame)
{
    DISCARD(frame);

    trace("fork()");

    task_t *new_task =
            sched_new_task(sched_alloc_pid(), sched_curr()->parent->pid, true);

    pmap_fork(&new_task->map.pmap, sched_curr()->parent->map.pmap);

    memcpy(new_task->files, sched_curr()->parent->files,
           sizeof(new_task->files));

    new_task->current_fd = sched_curr()->parent->current_fd;

    SLIST_INSERT_HEAD(&sched_curr()->children, new_task, link);

    new_task->parent = sched_curr();
    sched_curr()->children_n++;

    cpu_context_t ctx = sched_curr()->ctx;

    ctx.regs = *frame.frame;
    ctx.regs.rax = 0;

    sched_new_thread("forked thread", new_task, ctx, true);

    return new_task->pid;
}

static uintptr_t syscall_exec(syscall_frame_t frame)
{
    trace("exec(%s, %p, %p)", (char *)frame.param1, (void *)frame.param2,
          (void *)frame.param3);

    task_t *prev = sched_curr()->parent;

    task_t *new_process = sched_new_task(prev->pid, prev->ppid, true);

    new_process->parent = prev->parent;

    int r = sys_execve(new_process, (char *)frame.param1,
                       (const char **)frame.param2,
                       (const char **)frame.param3);

    if (r < 0)
        return r;

    thread_t *thread;

    sched_curr()->state = STOPPED;

    // Remove all threads from the calling process
    SLIST_FOREACH(thread, &prev->threads, task_link)
    {
        SLIST_REMOVE(&prev->threads, thread, thread, task_link);
    }

    memcpy(new_process->files, prev->files, sizeof(prev->files));
    new_process->current_fd = prev->current_fd;

    sched_tick(frame.frame);

    return frame.frame->rax;
}

#define W_EXITCODE(ret, sig) ((ret) << 8 | (sig))

static uintptr_t syscall_waitpid(syscall_frame_t frame)
{
    int pid = frame.param1;
    int *status = (int *)frame.param2;
    uint64_t child_pid = 0;

    if (sched_curr()->children_n == 0)
        return ECHILD;

    task_t *task;

    SLIST_FOREACH(task, &sched_curr()->children, link)
    {
        if (task->stopped && (pid == 0 || pid == task->pid)) {
            child_pid = task->pid;
            break;
        }
    }

    if (child_pid == 0) {
        sched_curr()->state = WAITING_FOR_CHILD;
        sched_tick(frame.frame);
    }

    sched_curr()->state = RUNNING;

    if (status)
        *status = W_EXITCODE(sched_curr()->child_status, 0);

    sched_curr()->child_exited = false;
    return sched_curr()->child_that_exited;
}

static uintptr_t syscall_ioctl(syscall_frame_t frame)
{
    return sys_ioctl(sched_curr()->parent, frame.param1, frame.param2,
                     (void *)frame.param3);
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
               0, (void *)req->window);
    } else if (actual_flags & MAP_ANONYMOUS) {
        vm_map(&sched_curr()->parent->map, NULL, req->size, prot, 0,
               (void *)req->window);
    }

    return 0;
}

static sys_handler *handlers[] = {
    [SYS_DEBUG] = syscall_debug,     [SYS_OPEN] = syscall_open,
    [SYS_CLOSE] = syscall_close,     [SYS_READ] = syscall_read,
    [SYS_WRITE] = syscall_write,     [SYS_SEEK] = syscall_seek,
    [SYS_MMAP] = syscall_mmap,       [SYS_EXIT] = syscall_exit,
    [SYS_STAT] = syscall_stat,       [SYS_TCB_SET] = syscall_tcb_set,
    [SYS_GETPID] = syscall_getpid,   [SYS_GETPPID] = syscall_getppid,
    [SYS_FORK] = syscall_fork,       [SYS_EXEC] = syscall_exec,
    [SYS_READDIR] = syscall_readdir, [SYS_WAITPID] = syscall_waitpid,
    [SYS_IOCTL] = syscall_ioctl,
};

uintptr_t syscall_handler(syscall_frame_t frame)
{
    return handlers[frame.num](frame);
}
