/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <asm.h>
#include <context.h>
#include <gaia/base.h>
#include <gaia/error.h>
#include <gaia/host.h>
#include <gaia/ports.h>
#include <gaia/rights.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/syscall.h>
#include <gaia/vm/vmm.h>

static Charon _charon;

static int sys_log(SyscallFrame frame)
{
    char *s = (char *)frame.first_arg;

    _log(LOG_NONE, NULL, "Debug [pid:%d] %s", sched_get_current_task()->pid, (char *)frame.first_arg);

    if (s[strlen(s) - 1] != '\n')
        host_debug_write_string("\n");

    return ERR_SUCCESS;
}

static int sys_alloc_port(SyscallFrame frame)
{
    uint32_t name = port_allocate(sched_get_current_task()->namespace, frame.first_arg);
    *frame.return_value = name;

    return ERR_SUCCESS;
}

static int sys_getpid(SyscallFrame frame)
{
    *frame.return_value = sched_get_current_task()->pid;
    return ERR_SUCCESS;
}

static int sys_free_port(SyscallFrame frame)
{
    port_free(sched_get_current_task()->namespace, frame.first_arg);
    return ERR_SUCCESS;
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

    if (args.flags & VM_MEM_DMA)
    {
        bool can_do_it = false;
        if (sched_get_current_task()->rights & RIGHT_DMA)
            can_do_it = true;

        if (!can_do_it)
        {
            return ERR_FORBIDDEN;
        }
    }

    *ret = vm_create(args);

    return ERR_SUCCESS;
}

static int sys_vm_map(SyscallFrame frame)
{
    void *space = (void *)frame.first_arg;
    VmMapArgs args = *(VmMapArgs *)frame.second_arg;

    if (args.flags & VM_MAP_DMA)
    {
        bool can_do_it = false;
        if (sched_get_current_task()->rights & RIGHT_DMA)
            can_do_it = true;

        if (!can_do_it)
        {
            return ERR_FORBIDDEN;
        }
    }

    if (space == NULL)
    {
        space = sched_get_current_task()->context->space;
    }

    int err = ERR_SUCCESS;

    if ((err = vm_map(space, args)) != ERR_SUCCESS)
    {
        return err;
    }

    return ERR_SUCCESS;
}

static int sys_create_task(SyscallFrame frame)
{
    if ((frame.second_arg & sched_get_current_task()->rights) != frame.second_arg)
    {
        return ERR_FORBIDDEN;
    }

    Task *task = sched_create_new_task(true, frame.second_arg);

    if (task)
    {
        host_accelerated_copy(task->namespace->well_known_ports, sched_get_current_task()->namespace->well_known_ports, sizeof(PortBinding) * WELL_KNOWN_PORTS_MAX);

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
        return ERR_FAILED;
    }

    struct user_task ret = {context_get_space(task->context), task->pid};

    *(struct user_task *)frame.first_arg = ret;

    return ERR_SUCCESS;
}

static int sys_start_task(SyscallFrame frame)
{
    struct user_task *user_task = (struct user_task *)frame.first_arg;
    Task *task = sched_lookup_task(user_task->pid);

    if (!task)
    {
        return ERR_INVALID_PARAMETERS;
    }

    context_start(task->context, frame.second_arg, frame.third_arg, frame.fourth_arg);

    VmCreateArgs args = {.addr = host_virt_to_phys((uintptr_t)gaia_get_charon()),
                         .flags = 0,
                         .size = ALIGN_UP(sizeof(Charon), 4096)};

    // FIXME: don't do this and instead make the bootstrap server give out the boot info
    VmObject object = vm_create(args);
    vm_map_phys(context_get_space(task->context), &object, args.addr, 0, VM_PROT_READ, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    task->context->frame.rdi = (uintptr_t)object.buf;

    task->state = RUNNING;
    return ERR_SUCCESS;
}

static int sys_yield(SyscallFrame frame)
{
    sched_tick(frame.int_frame);
    return ERR_SUCCESS;
}

static int sys_register_port(SyscallFrame frame)
{
    PortNamespace *ns = sched_get_current_task()->namespace;
    register_well_known_port(ns, frame.first_arg, ns->bindings.data[frame.second_arg]);
    return ERR_SUCCESS;
}

static int sys_get_port(SyscallFrame frame)
{
    *frame.return_value = sched_get_current_task()->namespace->well_known_ports[frame.first_arg].name;
    return ERR_SUCCESS;
}

static int sys_exit(SyscallFrame frame)
{
    sched_get_current_task()->state = STOPPED;

    log("Task %d exited with error code %d", sched_get_current_task()->pid, frame.first_arg);
    sched_tick(frame.int_frame);
    return ERR_SUCCESS;
}

static int sys_set_fs_base(SyscallFrame frame)
{
    asm_wrmsr(0xc0000100, (uint64_t)frame.first_arg);
    *frame.return_value = 0;
    return ERR_SUCCESS;
}

static int sys_msg(SyscallFrame frame)
{

    VmmMapSpace *space = context_get_space(sched_get_current_task()->context);

    //       if (frame.first_arg == PORT_SEND)
    //   log("send pid: %d, type: %d", sched_get_current_task()->pid, ((PortMessageHeader *)frame.fourth_arg)->type);
    *frame.return_value = port_msg(sched_get_current_task()->namespace, (uint8_t)frame.first_arg, (uint32_t)frame.second_arg, frame.third_arg, (PortMessageHeader *)frame.fourth_arg, &space);

    if (frame.first_arg == PORT_RECV && frame.return_value != 0 && space != context_get_space(sched_get_current_task()->context))
    {
        PortMessageHeader *header = (PortMessageHeader *)frame.fourth_arg;

        if (header->shmd_count > 0)
        {
            VmmMapSpace *current_space = context_get_space(sched_get_current_task()->context);
            VmmMapSpace *recv_space = space;

            for (size_t i = 0; i < header->shmd_count; i++)
            {
                VmObject obj;
                PortSharedMemoryDescriptor shmd = header->shmds[i];

                void *addr = NULL;

                if (((shmd.address) - ALIGN_DOWN(shmd.address, 4096)) + shmd.size > 4096)
                {
                    shmd.size = ALIGN_UP(((shmd.address) - ALIGN_DOWN(shmd.address, 4096)) + shmd.size, 4096);
                }

                for (size_t j = 0; j < ALIGN_UP(shmd.size, 4096) / 4096; j++)
                {
                    VmPhysBinding *binding = recv_space->phys_bindings;

                    while (binding)
                    {
                        if (shmd.address + j * 4096 >= binding->virt && shmd.address + j * 4096 < binding->virt + 4096)
                        {
                            break;
                        }
                        binding = binding->next;
                    }

                    if (!binding)
                    {
                        vmm_page_fault_handler(recv_space, shmd.address + j * 4096);
                        binding = recv_space->phys_bindings;

                        while (binding)
                        {
                            if (shmd.address + j * 4096 >= binding->virt && shmd.address + j * 4096 < binding->virt + 4096)
                            {
                                break;
                            }
                            binding = binding->next;
                        }
                    }

                    assert(binding);

                    VmCreateArgs args = {.addr = 0, .size = 4096, .flags = VM_MEM_DMA};
                    obj = vm_create(args);

                    vm_map_phys(current_space, &obj, binding->phys, 0, VM_PROT_READ | VM_PROT_WRITE, VM_MAP_ANONYMOUS | VM_MAP_DMA);

                    if (!addr)
                        addr = (void *)((uintptr_t)obj.buf + ((shmd.address + j * 4096) - binding->virt));
                }

                header->shmds[i].address = (uintptr_t)addr;
            }
        }
    }

    return ERR_SUCCESS;
}

static int sys_vm_write(SyscallFrame frame)
{
    vmm_write((void *)frame.first_arg, frame.second_arg, (void *)frame.third_arg, frame.fourth_arg);
    return ERR_SUCCESS;
}

static int sys_vm_register(SyscallFrame frame)
{
    VmmMapSpace *space = (VmmMapSpace *)frame.first_arg;
    uintptr_t address = frame.second_arg;
    size_t size = frame.third_arg;
    uint16_t flags = frame.fourth_arg;

    if (!(sched_get_current_task()->rights & RIGHT_REGISTER_DMA))
    {
        return ERR_FORBIDDEN;
    }

    if (!space)
    {
        space = context_get_space(sched_get_current_task()->context);
    }

    if (!vm_check_mappable_region(space, address, size))
    {
        return ERR_INVALID_PARAMETERS;
    }

    vm_new_mappable_region(space, address, size, flags);

    return ERR_SUCCESS;
}

static int sys_get_task(SyscallFrame frame)
{
    struct user_task *user_task = (struct user_task *)frame.first_arg;
    struct user_task task = {.space = context_get_space(sched_get_current_task()->context), .pid = sched_get_current_task()->pid};

    *user_task = task;
    *frame.return_value = 0;
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
    [GAIA_SYS_VM_REGISTER] = sys_vm_register,
    [GAIA_SYS_FREE_PORT] = sys_free_port,
    [GAIA_SYS_YIELD] = sys_yield,
    [GAIA_SYS_GETPID] = sys_getpid,
    [GAIA_SYS_GET_TASK] = sys_get_task,
    [GAIA_SYS_SET_FS_BASE] = sys_set_fs_base,
};

void syscall_init(Charon charon)
{
    _charon = charon;
}

void syscall(int num, SyscallFrame frame)
{
    *frame.errno = syscall_table[num](frame);
}
