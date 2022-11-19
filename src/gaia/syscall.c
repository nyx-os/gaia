/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/rights.h"
#include <context.h>
#include <gaia/base.h>
#include <gaia/host.h>
#include <gaia/ports.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/syscall.h>
#include <gaia/vmm.h>

static Charon _charon;

static int sys_log(SyscallFrame frame)
{
    _log(LOG_NONE, NULL, "\x1b[34m([%d])\x1b[0m %s", sched_get_current_task()->pid, (char *)frame.first_arg);
    return 0;
}

static int sys_alloc_port(SyscallFrame frame)
{
    uint32_t name = port_allocate(sched_get_current_task()->namespace, frame.first_arg);
    *frame.return_value = name;

    return 0;
}

struct PACKED user_task
{
    VmmMapSpace *space;
    size_t pid;
};

static int sys_vm_create(SyscallFrame frame)
{
    VmCreateArgs args = *(VmCreateArgs *)frame.first_arg;
    VmObject *ret = (VmObject *)frame.second_arg;

    if (args.flags & VM_MEM_PHYS)
    {
        bool can_do_it = false;
        if (sched_get_current_task()->rights & RIGHT_DMA)
            can_do_it = true;

        if (!can_do_it)
        {
            return -1;
        }
    }

    *ret = vm_create(args);

    return 0;
}

static int sys_vm_map(SyscallFrame frame)
{
    void *space = (void *)frame.first_arg;
    VmMapArgs args = *(VmMapArgs *)frame.second_arg;

    if (args.flags & VM_MAP_PHYS)
    {
        bool can_do_it = false;
        if (sched_get_current_task()->rights & RIGHT_DMA)
            can_do_it = true;

        if (!can_do_it)
        {
            panic("! You can't do DMA why are you trying to map phys memory nerd");
        }
    }

    if (space == NULL)
        space = sched_get_current_task()->context->space;

    *frame.return_value = (uint64_t)vm_map(space, args);

    return 0;
}

static int sys_create_task(SyscallFrame frame)
{
    Task *task = sched_create_new_task(true);

    if (task)
    {
        memcpy(task->namespace->well_known_ports, sched_get_current_task()->namespace->well_known_ports, sizeof(PortBinding) * WELL_KNOWN_PORTS_MAX);

        for (int i = 0; i < WELL_KNOWN_PORTS_MAX; i++)
        {
            PortBinding binding = task->namespace->well_known_ports[i];

            if (binding.name != PORT_NULL)
            {
                binding.name = task->namespace->current_name++;
                vec_push(&task->namespace->bindings, binding);
            }
        }

        task->rights = sched_get_current_task()->rights;
    }
    else
    {
        return -1;
    }

    struct user_task ret = {context_get_space(task->context), task->pid};

    *(struct user_task *)frame.first_arg = ret;

    return 0;
}

static int sys_start_task(SyscallFrame frame)
{
    struct user_task *user_task = (struct user_task *)frame.first_arg;
    Task *task = sched_lookup_task(user_task->pid);

    context_start(task->context, frame.second_arg, frame.third_arg, frame.fourth_arg);
    task->state = RUNNING;
    return 0;
}

static int sys_register_port(SyscallFrame frame)
{
    PortNamespace *ns = sched_get_current_task()->namespace;
    register_well_known_port(ns, frame.first_arg, ns->bindings.data[frame.second_arg]);
    return 0;
}

static int sys_get_port(SyscallFrame frame)
{
    *frame.return_value = sched_get_current_task()->namespace->well_known_ports[frame.first_arg].name;
    return 0;
}

static int sys_exit(SyscallFrame frame)
{
    sched_get_current_task()->state = STOPPED;

    log("Task %d exited", sched_get_current_task()->pid, frame.int_frame->rip);
    sched_tick(frame.int_frame);
    return 0;
}

static int sys_msg(SyscallFrame frame)
{
    *frame.return_value = port_msg(sched_get_current_task()->namespace, (uint8_t)frame.first_arg, (uint32_t)frame.second_arg, frame.third_arg, (PortMessageHeader *)frame.fourth_arg);
    return 0;
}

static int sys_vm_write(SyscallFrame frame)
{
    vmm_write((void *)frame.first_arg, frame.second_arg, (void *)frame.third_arg, frame.fourth_arg);
    return 0;
}

static int (*syscall_table[])(SyscallFrame) = {
    [GAIA_SYS_LOG] = sys_log,
    [GAIA_SYS_ALLOC_PORT] = sys_alloc_port,
    [GAIA_SYS_REGISTER_PORT] = sys_register_port,
    [GAIA_SYS_GET_PORT] = sys_get_port,
    [GAIA_SYS_EXIT] = sys_exit,
    [GAIA_SYS_MSG] = sys_msg,
    [GAIA_SYS_VM_MAP] = sys_vm_map,
    [GAIA_SYS_CREATE_TASK] = sys_create_task,
    [GAIA_SYS_START_TASK] = sys_start_task,
    [GAIA_SYS_VM_WRITE] = sys_vm_write,
    [GAIA_SYS_VM_CREATE] = sys_vm_create,
};

void syscall_init(Charon charon)
{
    _charon = charon;
}

void syscall(int num, SyscallFrame frame)
{
    syscall_table[num](frame);
}
