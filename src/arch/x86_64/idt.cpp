/* SPDX-License-Identifier: BSD-2-Clause */
#include <hal/int.hpp>
#include <kernel/ipl.hpp>
#include <kernel/main.hpp>
#include <kernel/sched.hpp>
#include <x86_64/asm.hpp>
#include <x86_64/idt.hpp>

namespace Gaia::x86_64 {

#define INTGATE 0x8e
#define TRAPGATE 0xef

struct [[gnu::packed]] IdtDescriptor {
  uint16_t size;
  uint64_t addr;
};

struct [[gnu::packed]] IdtEntry {
  uint16_t offset_lo;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_hi;
  uint32_t zero;
};

extern "C" uintptr_t __interrupt_vector[];
static IdtEntry idt[256] = {{0, 0, 0, 0, 0, 0, 0}};
static IdtEntry idt_make_entry(uint64_t offset, uint8_t type, uint8_t ring) {
  return (IdtEntry){
      .offset_lo = static_cast<uint16_t>(offset & 0xFFFF),
      .selector = 0x28,
      .ist = 0,
      .type_attr = (uint8_t)(type | ring << 5),
      .offset_mid = static_cast<uint16_t>((offset >> 16) & 0xFFFF),
      .offset_hi = static_cast<uint32_t>((offset >> 32) & 0xFFFFFFFF),
      .zero = 0};
}

static void install_isrs() {
  for (int i = 0; i < 256; i++) {
    idt[i] = idt_make_entry(__interrupt_vector[i], INTGATE, 0);
  }
}

static IdtDescriptor idtr = {};

static void idt_reload() {
  idtr.size = sizeof(idt) - 1;
  idtr.addr = (uintptr_t)idt;

  asm volatile("lidt %0" : : "m"(idtr));
}

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

extern "C" void timer_interrupt(void);

void idt_init() {
  install_isrs();
  idt_reload();

  idt[0x20] = idt_make_entry((uintptr_t)timer_interrupt, INTGATE, 0);

  // Disable PIC
  outb(PIC2_DATA, 0xff);
  outb(PIC1_DATA, 0xff);
}
} // namespace Gaia::x86_64
