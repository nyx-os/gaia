/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <context.h>
#include <gaia/base.h>
#include <gaia/host.h>
#include <gaia/ports.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/syscall.h>

static Charon _charon;

static void sys_log(SyscallFrame frame)
{
    _log(LOG_NONE, NULL, "\x1b[34m([%d])\x1b[0m %s", sched_get_current_task()->pid, (char *)frame.first_arg);
}

static void sys_alloc_port(SyscallFrame frame)
{
    uint32_t name = port_allocate(sched_get_current_task()->namespace, frame.first_arg);
    *frame.return_value = name;
}

static void sys_spawn(SyscallFrame frame)
{
    const char *name = (const char *)frame.first_arg;

    Task *task = NULL;

    for (int i = 0; i < _charon.modules.count; i++)
    {
        CharonModule module = _charon.modules.modules[i];

        if (strncmp(module.name, name, strlen(name)) == 0)
        {
            task = sched_create_new_task_from_elf((uint8_t *)module.address);
            break;
        }
    }

    if (task)
    {
        memcpy(task->namespace->well_known_ports, sched_get_current_task()->namespace->well_known_ports, sizeof(PortBinding) * WELL_KNOWN_PORTS_MAX);

        for (int i = 0; i < 4; i++)
        {
            PortBinding binding = task->namespace->well_known_ports[i];

            if (binding.name != PORT_NULL)
            {
                binding.name = task->namespace->current_name++;
                vec_push(&task->namespace->bindings, binding);
            }
        }
    }
}

static void sys_register_port(SyscallFrame frame)
{
    PortNamespace *ns = sched_get_current_task()->namespace;
    register_well_known_port(ns, frame.first_arg, ns->bindings.data[frame.second_arg]);
}

static void sys_get_port(SyscallFrame frame)
{
    *frame.return_value = sched_get_current_task()->namespace->well_known_ports[frame.first_arg].name;
}

static void sys_exit(SyscallFrame frame)
{
    sched_get_current_task()->state = STOPPED;

    log("Task %d exited", sched_get_current_task()->pid, frame.int_frame->rip);
    sched_tick(frame.int_frame);
}

static void sys_msg(SyscallFrame frame)
{
    port_msg(sched_get_current_task()->namespace, (uint8_t)frame.first_arg, (uint32_t)frame.second_arg, frame.third_arg, (PortMessageHeader *)frame.fourth_arg);
}

static void (*syscall_table[])(SyscallFrame) = {
    [GAIA_SYS_LOG] = sys_log,
    [GAIA_SYS_ALLOC_PORT] = sys_alloc_port,
    [GAIA_SYS_SPAWN] = sys_spawn,
    [GAIA_SYS_REGISTER_PORT] = sys_register_port,
    [GAIA_SYS_GET_PORT] = sys_get_port,
    [GAIA_SYS_EXIT] = sys_exit,
    [GAIA_SYS_MSG] = sys_msg,
};

void syscall_init(Charon charon)
{
    _charon = charon;
}

void syscall(int num, SyscallFrame frame)
{
    syscall_table[num](frame);
}
