/* SPDX-License-Identifier: BSD-2-Clause */
#include "sched.h"
#include "asm.h"
#include "machdep/intr.h"
#include "posix/fnctl.h"
#include <machdep/cpu.h>
#include <kern/sync.h>
#include <sys/queue.h>

static TAILQ_HEAD(, thread) runq;
static spinlock_t sched_lock;
static size_t ticks = 0;
static thread_t *current_thread = NULL, *idle_thread = NULL;
static bool restore_frame = false;
static pid_t curr_pid = 1;

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

pid_t sched_alloc_pid(void)
{
    return curr_pid++;
}

thread_t *sched_new_thread(const char *name, task_t *parent, cpu_context_t ctx,
                           bool insert)
{
    thread_t *new_thread = kmalloc(sizeof(thread_t));

    new_thread->ctx = ctx;
    new_thread->state = RUNNING;
    new_thread->parent = parent;
    new_thread->name = kmalloc(strlen(name) + 1);
    new_thread->child_exited = false;

    memcpy(new_thread->name, name, strlen(name));
    new_thread->name[strlen(name)] = 0;

    SLIST_INSERT_HEAD(&parent->threads, new_thread, task_link);

    if (insert) {
        TAILQ_INSERT_HEAD(&runq, new_thread, sched_link);
    }

    return new_thread;
}

task_t *sched_new_task(pid_t pid, pid_t ppid, bool user)
{
    task_t *new_task = kmalloc(sizeof(task_t));

    SLIST_INIT(&new_task->threads);

    new_task->pid = pid;
    new_task->ppid = ppid;
    new_task->current_fd = 0;

    if (user) {
        new_task->map.pmap = pmap_create(true);
    } else {
        new_task->map = vm_kmap;
    }

    LIST_INIT(&new_task->map.entries);

    vmem_init(&new_task->map.vmem, "task vmem", (void *)0x80000000000,
              0x100000000, 0x1000, 0, 0, 0, 0, 0);

    return new_task;
}

void idle_thread_fn(void)
{
    debug("Starting idle thread...");

    cpu_halt();
}

void sched_init(void)
{
    TAILQ_INIT(&runq);

    task_t *ktask = sched_new_task(-1, -1, false);

    SLIST_INIT(&ktask->threads);

    vaddr_t rsp = P2V(phys_allocz()) + PAGE_SIZE;

    cpu_context_t ctx = cpu_new_context((vaddr_t)idle_thread_fn, rsp, false);

    idle_thread = sched_new_thread("idle thread", ktask, ctx, false);

    current_thread = idle_thread;
}

void sched_idle(void)
{
    current_thread = idle_thread;
}

void sched_no_restore(void)
{
    restore_frame = false;
}

void sched_add_back(thread_t *thread, intr_frame_t *frame)
{
    TAILQ_INSERT_TAIL(&runq, thread, sched_link);
    (void)frame;
}

void sched_tick(intr_frame_t *ctx)
{
    spinlock_lock(&sched_lock);

    ticks++;

    if (restore_frame) {
        cpu_save_context(ctx, &current_thread->ctx);
    } else {
        restore_frame = true;
    }

    /* Slice ended, time to switch threads */
    if (ticks >= TIME_SLICE || current_thread->state == STOPPED) {
        if (current_thread->state == RUNNING && current_thread != idle_thread) {
            TAILQ_INSERT_TAIL(&runq, current_thread, sched_link);
        }

        current_thread = get_next_thread();
        current_thread->state = RUNNING;

        ticks = 0;
    }

    spinlock_unlock(&sched_lock);

    // log("running %s", current_thread->name);
    cpu_switch_context(ctx, current_thread->ctx);
    pmap_activate(current_thread->parent->map.pmap);
}

void sched_dump(void)
{
    thread_t *thread;
    log("Currently running threads: ");

    TAILQ_FOREACH(thread, &runq, sched_link)
    {
        log("- \"%s\"", thread->name);
    };
}

thread_t *sched_curr(void)
{
    return current_thread;
}
