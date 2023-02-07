/* @license:bsd2 */
#include <libkern/base.h>
#include <machdep/intr.h>
#include <x86_64/asm.h>

#define INTGATE 0x8e
#define TRAPGATE 0xef

typedef struct PACKED {
    uint16_t size;
    uint64_t addr;
} idt_pointer_t;

typedef struct PACKED {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t zero;
} idt_descriptor_t;

extern uintptr_t __interrupt_vector[];
static idt_descriptor_t idt[256] = { 0 };

static idt_descriptor_t idt_make_entry(uint64_t offset, uint8_t type)
{
    return (idt_descriptor_t){ .offset_lo = offset & 0xFFFF,
                               .selector = 0x28,
                               .ist = 0,
                               .type_attr = (uint8_t)(type),
                               .offset_mid = (offset >> 16) & 0xFFFF,
                               .offset_hi = (offset >> 32) & 0xFFFFFFFF,
                               .zero = 0 };
}

static void install_isrs(void)
{
    for (int i = 0; i < 256; i++) {
        idt[i] = idt_make_entry(__interrupt_vector[i], INTGATE);
    }
}

static idt_pointer_t idtr = { 0 };

static void idt_reload(void)
{
    idtr.size = sizeof(idt) - 1;
    idtr.addr = (uintptr_t)idt;

    asm volatile("lidt %0" : : "m"(idtr));
}

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

void idt_init(void)
{
    install_isrs();
    idt_reload();

    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void intr_register(int vec, intr_handler_t handler)
{
    idt[vec] = idt_make_entry((uintptr_t)handler, INTGATE);
}

uint64_t interrupts_handler(uint64_t rip)
{
    return rip;
}
