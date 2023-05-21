#include <machdep/intr.h>
#include <dev/ps2.h>
#include <libkern/base.h>
#include <asm.h>
#include <posix/tty.h>
#include <kern/term/term.h>

/* These conversion tables were stolen from lyre, all copyright goes to mintsuki */
static const char convtab_capslock[] = {
    '\0', '\033', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',    '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',    '[',  ']',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',    'L',  ';',  '\'', '`',  '\0', '\\', 'Z',  'X', 'C', 'V',
    'B',  'N',    'M',  ',',  '.',  '/',  '\0', '\0', '\0', ' '
};

static const char convtab_shift[] = {
    '\0', '\033', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',    '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',    '{',  '}',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',    'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X', 'C', 'V',
    'B',  'N',    'M',  '<',  '>',  '?',  '\0', '\0', '\0', ' '
};

static const char convtab_shift_capslock[] = {
    '\0', '\033', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',    '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',    '{',  '}',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',    'l',  ':',  '"',  '~',  '\0', '|',  'z',  'x', 'c', 'v',
    'b',  'n',    'm',  '<',  '>',  '?',  '\0', '\0', '\0', ' '
};

static const char convtab_nomod[] = {
    '\0', '\033', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',    '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',    '[',  ']',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',    'l',  ';',  '\'', '`',  '\0', '\\', 'z',  'x', 'c', 'v',
    'b',  'n',    'm',  ',',  '.',  '/',  '\0', '\0', '\0', ' '
};

static bool shift = false;
static bool caps_lock = false;

void ps2_handler(intr_frame_t *frame)
{
    DISCARD(frame);

    uint8_t code = inb(0x60);

    switch (code) {
    case PS2_SCANCODE_SHIFT_LEFT:
    case PS2_SCANCODE_SHIFT_RIGHT:
        shift = true;
        break;
    case PS2_SCANCODE_SHIFT_LEFT_REL:
    case PS2_SCANCODE_SHIFT_RIGHT_REL:
        shift = false;
        break;

    case PS2_SCANCODE_CAPSLOCK:
        caps_lock = !caps_lock;
        break;
    default: {
        if (code < PS2_SCANCODE_MAX) {
            const char *convtab = convtab_nomod;

            if (shift && caps_lock)
                convtab = convtab_shift_capslock;
            else if (shift) {
                convtab = convtab_shift;
            } else if (caps_lock) {
                convtab = convtab_capslock;
            }

            tty_input(convtab[code]);
        }

        break;
    }
    }
}

/* TODO: detect if ps2 is present using ACPI or something */

void ps2_init(void)
{
#ifdef __x86_64__
    intr_register(33, ps2_handler);
#else
    assert(!"PS2 on non-x86 is not supported");
#endif
}
