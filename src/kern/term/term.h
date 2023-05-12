/* @license:bsd2 */
#ifndef SRC_KERN_TERM_H
#define SRC_KERN_TERM_H
#include <kern/charon.h>

void term_init(charon_t charon);
void term_write(const char *string);

#endif
