#ifndef POSIX_TTY_H
#define POSIX_TTY_H
#include <termios.h>
#include <libkern/base.h>

#define TTY_NUM 1

typedef struct {
    /* Lines can be at maximum 4096 characters long, so this is good enough, I think */
    char buf[4096];
    size_t read_cursor, write_cursor, buf_length, line_count;
    struct termios termios;
} tty_t;

void tty_init(void);
void tty_input(char c);

#endif /* POSIX_TTY_H */
