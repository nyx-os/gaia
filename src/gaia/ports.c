#include <gaia/ports.h>
#include <gaia/slab.h>

static Vec(Port) ports;

uint32_t port_allocate(PortNamespace *ns, uint8_t rights)
{
    Port new_port = {{0}};

    vec_push(&ports, new_port);

    // Add new port binding to namespace
    PortBinding binding = {ns->current_name++, rights, ports.length - 1, false};

    vec_push(&ns->bindings, binding);

    return binding.name;
}

void port_free(PortNamespace *ns, uint32_t name)
{
    for (size_t i = 0; i < ns->bindings.length; i++)
    {
        if (ns->bindings.data[i].name == name)
        {
            for (size_t j = i; j < ns->bindings.length; ++j)
                ns->bindings.data[j] = ns->bindings.data[j + 1];

            ns->bindings.length--;
            break;
        }
    }
}

void port_send(PortNamespace *ns, PortMessageHeader *message)
{
    Port *port = NULL;

    if (message->dest < 0 || message->dest >= ns->bindings.length)
    {
        panic("Name %d not found in namespace", message->dest);
    }

    PortBinding binding = ns->bindings.data[message->dest];

    port = &ports.data[binding.port];

    if (!port)
    {
        panic("Name %d not found in namespace", message->dest);
    }

    if (!(binding.rights & PORT_RIGHT_SEND))
    {
        panic("Holder of port rights %d does not have send rights", message->dest);
    }

    port->queue.messages[port->queue.head] = slab_alloc(message->size);
    memcpy(port->queue.messages[port->queue.head], message, message->size);

    if (message->type == PORT_MSG_TYPE_RIGHT)
    {
        port->queue.messages[port->queue.head]->kernel_data.port = ns->bindings.data[message->port_right].port;
    }

    if (message->type == PORT_MSG_TYPE_RIGHT_ONCE)
    {
        port->queue.messages[port->queue.head]->kernel_data.port = ns->bindings.data[message->port_right].port;
        ns->bindings.data[message->port_right].send_once = true;
    }

    if (binding.send_once)
    {

        // Find and remove the binding from the namespace
        port_free(ns, message->dest);
    }

    port->queue.head = (port->queue.head + 1) & (PORT_QUEUE_MAX - 1);
}

PortMessageHeader *port_receive(PortNamespace *ns, uint32_t name)
{
    Port *port = NULL;
    PortBinding binding = ns->bindings.data[name];

    port = &ports.data[binding.port];

    if (!port)
    {
        panic("Name %d not found in namespace", name);
    }

    if (!(binding.rights & PORT_RIGHT_RECV))
    {
        panic("Holder of port rights %d does not have receive rights", name);
    }

    PortMessage *ret = port->queue.messages[port->queue.tail];

    if (!ret)
    {
        return NULL;
    }

    if (ret->header.type == PORT_MSG_TYPE_RIGHT)
    {
        PortBinding binding = {0};
        binding.name = ns->current_name++;
        binding.rights = ret->header.port_right;
        binding.port = ret->kernel_data.port;

        vec_push(&ns->bindings, binding);
        ret->header.port_right = binding.name;
    }

    else if (ret->header.type == PORT_MSG_TYPE_RIGHT_ONCE)
    {
        PortBinding binding = {0};
        binding.name = ns->current_name++;
        binding.rights = ret->header.port_right;
        binding.port = ret->kernel_data.port;
        binding.send_once = true;

        vec_push(&ns->bindings, binding);
        ret->header.port_right = binding.name;
    }

    port->queue.tail = (port->queue.tail + 1) & (PORT_QUEUE_MAX - 1);

    return &ret->header;
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

size_t port_msg(PortNamespace *ns, uint8_t type, uint32_t port_to_receive, size_t bytes_to_receive, PortMessageHeader *header)
{
    if (type == PORT_SEND)
    {
        port_send(ns, header);
        return header->size;
    }

    else if (type == PORT_RECV)
    {
        PortMessageHeader *ret = port_receive(ns, port_to_receive);

        if (ret != NULL)
        {
            memcpy(header, ret, bytes_to_receive);
            return ret->size;
        }
    }

    return 0;
}
