/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_MACHDEP_MACHDEP_H_
#define SRC_ARCH_X86_64_MACHDEP_MACHDEP_H_

void machine_dbg_putc(int c, void *ctx);
void machine_init(void);
void machine_init_devices(void);

#endif
