#include "dev/console/fbconsole.hpp"
#include <amd64/asm.hpp>
#include <dev/acpi/acpi.hpp>
#include <dev/acpi/device.hpp>
#include <dev/console/ps2.hpp>
#include <hal/int.hpp>
#include <lai/core.h>
#include <lai/helpers/resource.h>
#include <lib/log.hpp>
#include <linux/input-event-codes.h>

namespace Gaia::Dev {

static const char convtab_capslock[] = {
    '\0', '\033', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',    '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',    '[',  ']',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',    'L',  ';',  '\'', '`',  '\0', '\\', 'Z',  'X', 'C', 'V',
    'B',  'N',    'M',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

static const char convtab_shift[] = {
    '\0', '\033', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',    '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',    '{',  '}',  '\r', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',    'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X', 'C', 'V',
    'B',  'N',    'M',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};

static const char convtab_shift_capslock[] = {
    '\0', '\033', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',    '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',    '{',  '}',  '\r', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',    'l',  ':',  '"',  '~',  '\0', '|',  'z',  'x', 'c', 'v',
    'b',  'n',    'm',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};

static const char convtab_nomod[] = {
    '\0', '\033', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',    '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',    '[',  ']',  '\r', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',    'l',  ';',  '\'', '`',  '\0', '\\', 'z',  'x', 'c', 'v',
    'b',  'n',    'm',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

Vm::UniquePtr<Service> Ps2Controller::init() {
  return {Vm::get_allocator(), new Ps2Controller};
}

static uint8_t read_ps2() {
  while (!(Amd64::inb(0x64) & 1)) {
  }
  return Amd64::inb(0x60);
}

void Ps2Controller::int_handler(Hal::InterruptFrame *frame, void *arg) {
  (void)frame;

  uint8_t code = Amd64::inb(0x60);

  auto controller = (Ps2Controller *)arg;

  if (system_console() && system_console()->tty->in_mediumraw) {
    system_console()->input(code);
  }

  switch (code) {
  case Scancode::SHIFT_LEFT:
  case Scancode::SHIFT_RIGHT:
    controller->shift = true;
    break;
  case Scancode::SHIFT_LEFT_REL:
  case Scancode::SHIFT_RIGHT_REL:
    controller->shift = false;
    break;

  case Scancode::CTRL:
    controller->ctrl = true;
    break;
  case Scancode::CTRL_REL:
    controller->ctrl = false;
    break;

  case Scancode::CAPSLOCK:
    controller->capslock = !controller->capslock;
    break;

  default: {
    if (code < Scancode::MAX) {
      const char *convtab = convtab_nomod;

      if (controller->shift && controller->capslock)
        convtab = convtab_shift_capslock;
      else if (controller->shift) {
        convtab = convtab_shift;
      } else if (controller->capslock) {
        convtab = convtab_capslock;
      }

      char c = convtab[code];

      if (controller->ctrl) {
        c = toupper(c) - 0x40;
      }

      // This kinda sucks
      if (system_console() && system_console()->tty->in_mediumraw == false) {
        system_console()->input(c);
      }
    }

    break;
  }
  }
}

static void write_ps2(uint16_t port, uint8_t value) {
  while (Amd64::inb(0x64) & 2) {
    asm volatile("pause");
  }
  Amd64::outb(port, value);
}

void Ps2Controller::start(Service *provider) {
  attach(provider);

  log("Hi PS/2");

  frg::output_to(name_str) << frg::fmt("{}", class_name());

  acpi_device = (AcpiDevice *)provider;

  // TODO: use gsi from acpi instead
  Hal::register_interrupt_handler(33, int_handler, this, &int_entry);

  // stolen from vinix
  write_ps2(0x64, 0xad);
  write_ps2(0x64, 0xa7);

  while ((Amd64::inb(0x64) & 1) != 0) {
    Amd64::inb(0x60);
  }

  write_ps2(0x64, 0x20);
  auto conf = read_ps2();

  conf |= (1 << 0) | (1 << 6);
  write_ps2(0x64, 0x60);
  write_ps2(0x60, conf);
  write_ps2(0x64, 0xae);
}

void ps2_driver_register() {
  auto props = new Properties{frg::hash<frg::string_view>{}};

  props->insert("hid", {.string = "PNP0303"});
  props->insert("altHid", {.string = "PNP0F13"});
  props->insert("provider", {.string = "AcpiDevice"});
  get_catalog().register_driver(Driver{&Ps2Controller::init}, *props);
}

} // namespace Gaia::Dev
