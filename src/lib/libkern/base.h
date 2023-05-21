/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_LIB_LIBKERN_BASE_H_
#define SRC_LIB_LIBKERN_BASE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <libkern/string.h>
#include <kern/vm/liballoc.h>
#include <libkern/nanoprintf.h>
#include <libkern/debug.h>
#include <machdep/machdep.h>
#include <machdep/vm.h>
#include <machdep/cpu.h>

#define krealloc(p, _, n) liballoc_krealloc(p, n)
#define kmalloc liballoc_kmalloc
#define kfree(p, n) liballoc_kfree(p)
#define kprintf(...) npf_pprintf(machine_dbg_putc, NULL, __VA_ARGS__)

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#define DIV_CEIL(x, align) (((x) + (align)-1) / (align))
#define PACKED __attribute__((packed))
#define UNUSED __attribute__((unused))
#define DISCARD(x) ((void)(x))

#define asm __asm__

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define KIB(x) ((uint64_t)x << 10)
#define MIB(x) ((uint64_t)x << 20)
#define GIB(x) ((uint64_t)x << 30)

#define VOLATILE_WRITE(type, addr, val) (*(volatile type *)(addr)) = val
#define VOLATILE_READ(type, addr) (*(volatile type *)(addr))

#define assert(...)                                        \
    {                                                      \
        if (!(__VA_ARGS__))                                \
            panic("assertion failed: %s\n", #__VA_ARGS__); \
    }

#endif
