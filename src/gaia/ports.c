#include "gaia/host.h"
#include <gaia/ports.h>
#include <gaia/slab.h>
#include <gaia/vm/vmm.h>

typedef struct port_list_item
{
    Port data;
    struct port_list_item *prev, *next;
} PortListItem;

static PortListItem *port_list_head = NULL;

uint32_t port_allocate(PortNamespace *ns, uint8_t rights)
{
    Port new_port = {1, {0}};

    PortListItem *new_item = malloc(sizeof(PortListItem));

    new_item->data = new_port;
    new_item->next = port_list_head;

    if (port_list_head)
        port_list_head->prev = new_item;

    port_list_head = new_item;

    // Add new port binding to namespace
    PortBinding binding = {ns->current_name++, rights, new_item, false};

    // log("Pushed binding with (alloc) %p", binding.port);
    vec_push(&ns->bindings, binding);

    return binding.name;
}

void port_free(PortNamespace *ns, uint32_t name)
{
    for (size_t i = 0; i < ns->bindings.length; i++)
    {
        if (ns->bindings.data[i].name == name)
        {
            PortListItem *port_item = ns->bindings.data[i].port;

            for (size_t j = i; j < ns->bindings.length; ++j)
                ns->bindings.data[j] = ns->bindings.data[j + 1];

            ns->bindings.length--;

            if (port_item->data.ref_count > 0)
                port_item->data.ref_count--;

            if (!port_item->data.ref_count)
            {
                for (int i = 0; i < port_item->data.queue.length; i++)
                {
                    free(port_item->data.queue.messages[i].header);
                }

                free(port_item);
            }

            break;
        }
    }
}

void port_send(PortNamespace *ns, PortMessageHeader *message, VmmMapSpace **space)
{
    Port *port = NULL;

    if (message->dest < 0)
    {
        panic("Name %d not found in namespace", message->dest);
    }

    PortBinding binding = {0};

    for (size_t i = 0; i < ns->bindings.length; i++)
    {
        if (ns->bindings.data[i].name == message->dest)
        {
            binding = ns->bindings.data[i];
            break;
        }
    }

    port = &((PortListItem *)(binding.port))->data;

    if (!port)
    {
        panic("Name %d not found in namespace (port is null)", message->dest);
    }

    if (!(binding.rights & PORT_RIGHT_SEND))
    {
        panic("Holder of port rights %d does not have send rights", message->dest);
    }

    port->queue.messages[port->queue.head].header = malloc(message->size);
    port->queue.length++;

    if (message->shmd_count > 0)
    {
        port->queue.messages[port->queue.head].kernel_data.space = *space;
    }

    host_accelerated_copy(port->queue.messages[port->queue.head].header, message, message->size);

    if (message->type == PORT_MSG_TYPE_RIGHT)
    {

        PortBinding port_right_binding = {0};
        for (size_t i = 0; i < ns->bindings.length; i++)
        {
            if (ns->bindings.data[i].name == message->port_right)
            {
                port_right_binding = ns->bindings.data[i];
                break;
            }
        }

        port->queue.messages[port->queue.head].kernel_data.port = port_right_binding.port;
    }

    if (message->type == PORT_MSG_TYPE_RIGHT_ONCE)
    {
        PortBinding port_right_binding = {0};
        size_t index = 0;
        for (size_t i = 0; i < ns->bindings.length; i++)
        {
            if (ns->bindings.data[i].name == message->port_right)
            {
                port_right_binding = ns->bindings.data[i];
                index = i;
                break;
            }
        }

        port->queue.messages[port->queue.head].kernel_data.port = port_right_binding.port;
        ns->bindings.data[index].send_once = true;
    }

    port->queue.head = (port->queue.head + 1) & (PORT_QUEUE_MAX - 1);
}

PortMessageHeader *port_receive(PortNamespace *ns, uint32_t name, VmmMapSpace **space)
{
    Port *port = NULL;
    PortBinding binding = {0};

    for (size_t i = 0; i < ns->bindings.length; i++)
    {
        if (ns->bindings.data[i].name == name)
        {
            binding = ns->bindings.data[i];
            break;
        }
    }

    port = &((PortListItem *)(binding.port))->data;

    if (!port)
    {
        panic("Name %d not found in namespace (recv)", name);
    }

    if (!(binding.rights & PORT_RIGHT_RECV))
    {
        panic("Holder of port rights %d (kernel port = %d) does not have receive rights", name, binding.port);
    }

    PortMessage ret = port->queue.messages[port->queue.tail];

    if (!ret.header)
    {
        return NULL;
    }

    if (ret.header->shmd_count > 0)
    {
        *space = ret.kernel_data.space;
    }

    if (ret.header->type == PORT_MSG_TYPE_RIGHT)
    {
        PortBinding binding = {0};
        binding.name = ns->current_name++;
        binding.rights = PORT_RIGHT_SEND;
        binding.port = ret.kernel_data.port;
        ((PortListItem *)(ret.kernel_data.port))->data.ref_count++;

        vec_push(&ns->bindings, binding);

        ret.header->port_right = binding.name;
    }

    else if (ret.header->type == PORT_MSG_TYPE_RIGHT_ONCE)
    {
        PortBinding binding = {0};

        binding.name = ns->current_name++;
        binding.rights = PORT_RIGHT_SEND;

        binding.port = ret.kernel_data.port;

        binding.send_once = true;

        vec_push(&ns->bindings, binding);

        ret.header->port_right = binding.name;
    }

    port->queue.length--;

    port->queue.tail = (port->queue.tail + 1) & (PORT_QUEUE_MAX - 1);

    if (binding.send_once)
    {
        // Remove port
        port_free(ns, binding.name);
    }

    return ret.header;
}

void register_well_known_port(PortNamespace *ns, uint8_t index, PortBinding binding)
{
    if (ns->well_known_ports[index].port != 0 || ns->well_known_ports[index].name != 0 || ns->well_known_ports[index].rights != 0)
    {
        panic("Well known port %d already registered", index);
    }

    ns->well_known_ports[index] = binding;

    if (binding.rights & PORT_RIGHT_RECV && binding.rights & PORT_RIGHT_SEND)
    {
        ns->well_known_ports[index].rights = PORT_RIGHT_SEND;
    }

    else if (binding.rights & PORT_RIGHT_RECV && !(binding.rights & PORT_RIGHT_SEND))
    {
        panic("Cannot register a receive right");
    }
}

size_t port_msg(PortNamespace *ns, uint8_t type, uint32_t port_to_receive, size_t bytes_to_receive, PortMessageHeader *header, VmmMapSpace **space)
{
    if (type == PORT_SEND)
    {
        port_send(ns, header, space);
        return header->size;
    }

    else if (type == PORT_RECV)
    {
        PortMessageHeader *ret = port_receive(ns, port_to_receive, space);

        if (ret != NULL)
        {
            host_accelerated_copy(header, ret, bytes_to_receive);
            return ret->size;
        }
    }

    return 0;
}
