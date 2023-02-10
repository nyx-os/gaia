/* SPDX-License-Identifier: BSD-2-Clause */
#include "sched.h"
#include <kern/sync.h>

static TAILQ_HEAD(, thread) runq;
static spinlock_t sched_lock;
static size_t ticks = 0;
static thread_t *current_thread = NULL, *idle_thread = NULL;
static bool restore_frame = false;

static thread_t *get_next_thread(void)
{
    thread_t *ret;

    ret = TAILQ_FIRST(&runq);

    if (!ret) {
        ret = idle_thread;
    } else {
        TAILQ_REMOVE(&runq, ret, sched_link);
    }

    return ret;
}

thread_t *sched_new_thread(const char *name, task_t *parent, intr_frame_t ctx)
{
    thread_t *new_thread = kmalloc(sizeof(thread_t));

    new_thread->ctx = ctx;
    new_thread->state = RUNNING;
    new_thread->parent = parent;
    new_thread->name = name;

    SLIST_INSERT_HEAD(&parent->threads, new_thread, task_link);
    TAILQ_INSERT_HEAD(&runq, new_thread, sched_link);

    return new_thread;
}

task_t *sched_new_task(pid_t pid)
{
    task_t *new_task = kmalloc(sizeof(task_t));

    SLIST_INIT(&new_task->threads);

    new_task->pid = pid;
    new_task->map.pmap = pmap_create();
    vmem_init(&new_task->map.vmem, "task vmem", NULL, 0, 0, 0, 0, 0, 0, 0);
    return new_task;
}

void idle_thread_fn(void)
{
    while (true) {
        asm volatile("outb %0, $0xe9" ::"a"((char)'a'));
        asm volatile("hlt");
    }
}

void idle_thread_fn2(void)
{
    while (true) {
        asm volatile("hlt");
    }
}

void sched_init(void)
{
    TAILQ_INIT(&runq);

    task_t ktask = { 0 };
    SLIST_INIT(&ktask.threads);
    ktask.map = vm_kmap;
    ktask.pid = 0;

    intr_frame_t ctx = { .ss = 0x30,
                         .cs = 0x28,
                         .rflags = 0x202,
                         .rip = (uintptr_t)idle_thread_fn,
                         .rsp = P2V(phys_allocz()) + 4096 };

    idle_thread = sched_new_thread("idle thread", &ktask, ctx);

    intr_frame_t ctx2 = { .ss = 0x30,
                          .cs = 0x28,
                          .rflags = 0x202,
                          .rip = (uintptr_t)idle_thread_fn2,
                          .rsp = P2V(phys_allocz()) + 4096 };

    sched_new_thread("idle thread2", &ktask, ctx2);

    current_thread = idle_thread;
}

void sched_tick(intr_frame_t *ctx)
{
    spinlock_lock(&sched_lock);

    ticks++;

    if (restore_frame) {
        current_thread->ctx = *ctx;
    } else {
        restore_frame = true;
    }

    /* Slice ended, time to switch threads */
    if (ticks > TIME_SLICE) {
        if (current_thread->state == RUNNING) {
            TAILQ_INSERT_TAIL(&runq, current_thread, sched_link);
        }

        current_thread = get_next_thread();
        current_thread->state = RUNNING;

        ticks = 0;
    }

    *ctx = current_thread->ctx;
    pmap_activate(current_thread->parent->map.pmap);

    spinlock_unlock(&sched_lock);
}
