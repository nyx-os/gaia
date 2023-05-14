/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file sched.h
 * @brief Task scheduler
 *
 * This is the OS's task scheduler, using a simple round-robin algorithm.
 */

#ifndef SRC_KERN_SCHED_H_
#define SRC_KERN_SCHED_H_
#include <machdep/intr.h>
#include <machdep/cpu.h>
#include <kern/vm/vm.h>
#include <posix/fs/fd.h>

/**
 * Time slice to use, in ticks
 */
#define TIME_SLICE 5

struct task;

enum thread_state {
    RUNNING,
    BLOCKED,
    WAITING_FOR_CHILD,
    STOPPED,
};

/**
 * A single unit of execution, contains the CPU information like registers
 */
typedef struct thread {
    cpu_context_t ctx; /**< CPU context */
    enum thread_state state; /**< Current state of the thread */
    const char *name; /**< Name of the thread */

    struct task *parent; /**< Back pointer to parent process */

    SLIST_ENTRY(thread) task_link; /**< Linkage into task_t::threads */
    TAILQ_ENTRY(thread) sched_link; /**< Linkage into the scheduler's queue */

} thread_t;

/**
 * A collection of resources, also called a process by other operating systems
 */
typedef struct task {
    vm_map_t map; /**< Virtual address space for the process, shared across all threads */
    pid_t pid, ppid; /**< Process ID */
    SLIST_HEAD(, thread) threads; /**< Threads that are attached to the task */
    SLIST_HEAD(, task) children; /**< Child processes */
    fd_t *files[64]; /**< FD table */
    uint8_t current_fd; /** Current fd */
    vnode_t *cwd; /**< Current working directory */
    SLIST_ENTRY(task) link;
} task_t;

void sched_init(void);
void sched_tick(intr_frame_t *ctx);

thread_t *sched_new_thread(const char *name, task_t *parent, cpu_context_t ctx,
                           bool insert);
task_t *sched_new_task(pid_t pid, pid_t ppid, bool user);

thread_t *sched_curr(void);

void sched_dump(void);

void sched_idle(void);

pid_t sched_alloc_pid(void);

void sched_no_restore(void);

#endif
