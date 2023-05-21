#include "libkern/base.h"
#include "machdep/cpu.h"
#include <posix/fs/vfs.h>
#include <kern/vm/vm.h>
#include <asm.h>
#include <posix/fs/cdev.h>
#include <kern/term/term.h>
#include <posix/tty.h>

tty_t *ttys[TTY_NUM];

static int tty_push(tty_t *tty, char c)
{
    if (tty->buf_length == ARRAY_LENGTH(tty->buf)) {
        return -1;
    }

    tty->buf[tty->write_cursor++] = c;

    if (tty->write_cursor == ARRAY_LENGTH(tty->buf)) {
        tty->write_cursor = 0;
    }

    if (c == tty->termios.c_cc[VEOL])
        tty->line_count++;

    tty->buf_length++;

    return 0;
}

static char tty_remove_last(tty_t *tty)
{
    size_t prev_index = 0;
    char c;

    if (tty->buf_length == 0) {
        return -1;
    }

    if (tty->write_cursor == 0) {
        prev_index = ARRAY_LENGTH(tty->buf) - 1;
    } else {
        prev_index = tty->write_cursor - 1;
    }

    c = tty->buf[prev_index];

    if (c == tty->termios.c_cc[VEOL]) {
        return 0;
    }

    tty->buf_length--;
    tty->write_cursor = prev_index;
    return c;
}

static char tty_pop(tty_t *tty)
{
    char c;

    if (tty->buf_length == 0) {
        return 0;
    }

    c = tty->buf[tty->read_cursor++];

    if (tty->read_cursor == ARRAY_LENGTH(tty->buf)) {
        tty->read_cursor = 0;
    }

    if (c == tty->termios.c_cc[VEOL])
        tty->line_count--;

    tty->buf_length--;
    return c;
}

void tty_input(char c)
{
    tty_t *tty = ttys[0];

    bool is_canon = tty->termios.c_lflag & ICANON;

    // Convert NL to CR
    if (c == '\n' && tty->termios.c_iflag & INLCR) {
        c = '\r';
    }
    // Convert CR to NL
    else if (c == '\r' && tty->termios.c_iflag & ICRNL) {
        c = '\n';
    }

    // Ignore CR
    else if (c == '\r' && tty->termios.c_iflag & IGNCR) {
        return;
    }

    if (is_canon) {
        // If the erase key is pressed
        if (c == tty->termios.c_cc[VERASE]) {
            if (tty_remove_last(tty) >= 0)
                term_write("\b \b");
            return;
        }
    } else {
        panic("TODO: noncanonical mode in terminal is not implemented");
    }

    if (tty->termios.c_lflag & ECHO ||
        ((c == '\n' && tty->termios.c_lflag & ECHONL) && is_canon)) {
        term_putchar(c);
    }

    tty_push(tty, c);
}

static int tty_read(dev_t dev, void *buf, size_t nbyte, size_t off)
{
    DISCARD(dev);
    DISCARD(off);

    log("tty_read()");

    tty_t *tty = ttys[0];
    char *cbuf = (char *)buf;
    size_t i;
    while (!tty->line_count) {
        cpu_enable_interrupts();
    }

    if (nbyte > tty->buf_length) {
        nbyte = tty->buf_length;
    }

    for (i = 0; i < nbyte; i++) {
        char c = tty_pop(tty);
        cbuf[i] = c;

        if (tty->termios.c_lflag & ICANON && c == tty->termios.c_cc[VEOL]) {
            break;
        }
    }

    return i + 1;
}

static int tty_write(dev_t dev, void *buf, size_t nbyte, size_t off)
{
    DISCARD(dev);
    DISCARD(off);

    char *cbuf = buf;

    for (size_t i = 0; i < nbyte; i++) {
        term_putchar(cbuf[i]);
    }

    return nbyte;
}

void tty_init(void)
{
    vnode_t *vn = NULL;

    cdev_t cdev = {
        .is_atty = true,
        .read = tty_read,
        .write = tty_write,
    };

    tty_t *tty = kmalloc(sizeof(tty_t));

    root_devnode->ops.mknod(root_devnode, &vn, "tty",
                            MKDEV(cdev_allocate(&cdev), 0));

    tty->termios.c_cc[VEOL] = '\n';
    tty->termios.c_cc[VEOF] = 0;
    tty->termios.c_cc[VERASE] = '\b';
    tty->termios.c_cflag = (CREAD | CS8 | HUPCL);
    tty->termios.c_iflag = (BRKINT | ICRNL | IMAXBEL | IXON | IXANY);
    tty->termios.c_lflag = (ECHO | ICANON | ISIG | IEXTEN | ECHOE);
    tty->termios.c_oflag = (OPOST | ONLCR);

    ttys[0] = tty;
}
