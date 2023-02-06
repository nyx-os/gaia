/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_LIB_LIBKERN_DEBUG_H_
#define SRC_LIB_LIBKERN_DEBUG_H_
#include <libkern/base.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\e[1m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_CYAN "\x1b[36m"

#define __FILENAME__                                                           \
    (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : \
                                        __FILE__)

#define log(...)                                             \
    do {                                                     \
        kprintf(ANSI_GREEN "%s: " ANSI_RESET, __FILENAME__); \
        kprintf(__VA_ARGS__);                                \
        kprintf("\n");                                       \
    } while (0)

#define panic(...)                                         \
    do {                                                   \
        kprintf(ANSI_RED "%s: " ANSI_RESET, __FILENAME__); \
        kprintf(__VA_ARGS__);                              \
        kprintf("\n");                                     \
        cpu_halt();                                        \
    } while (0)

#define trace(...)                                          \
    do {                                                    \
        kprintf(ANSI_CYAN "%s: " ANSI_RESET, __FILENAME__); \
        kprintf(__VA_ARGS__);                               \
        kprintf("\n");                                      \
    } while (0)

#endif
