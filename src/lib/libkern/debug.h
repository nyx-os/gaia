/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_LIB_LIBKERN_DEBUG_H_
#define SRC_LIB_LIBKERN_DEBUG_H_

/*
  0 - Don't show logs at all
  1 - Show error() only
  2 - Show warn()
  3 - Show info()
  4 - Show trace()
  5 - Show debug()
*/
#define LOG_VERBOSITY 5

#define LOG_COLORS 1

#if LOG_COLORS == 1
#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\e[1m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RED "\x1b[31m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_BLUE "\x1b[34m"
#else
#define ANSI_RESET ""
#define ANSI_BOLD ""
#define ANSI_GREEN ""
#define ANSI_RED ""
#define ANSI_CYAN ""
#define ANSI_BLUE
#endif

#define __FILENAME__                                                           \
    (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : \
                                        __FILE__)

#define DO_LOG_FN(LEVEL, COLOR, ...)                                           \
    kprintf(COLOR LEVEL ANSI_RESET " \x1b[90m%s:%d " ANSI_RESET, __FILENAME__, \
            __LINE__);                                                         \
    kprintf(__VA_ARGS__);                                                      \
    kprintf("\n");

#if LOG_VERBOSITY >= 1
#define error(...)                                \
    do {                                          \
        DO_LOG_FN("error", ANSI_RED, __VA_ARGS__) \
    } while (0)
#define panic(...)                                \
    do {                                          \
        DO_LOG_FN("panic", ANSI_RED, __VA_ARGS__) \
        cpu_halt();                               \
    } while (0)
#else
#define error(...)
#define panic(...)
#endif

#if LOG_VERBOSITY >= 2
#define warn(...)                                    \
    do {                                             \
        DO_LOG_FN("warn ", ANSI_YELLOW, __VA_ARGS__) \
    } while (0)

#else
#define warn(...)
#endif

#if LOG_VERBOSITY >= 3
#define log(...)                                    \
    do {                                            \
        DO_LOG_FN("info ", ANSI_GREEN, __VA_ARGS__) \
    } while (0)
#else
#define log(...)
#endif

#if LOG_VERBOSITY >= 4
#define trace(...)                                 \
    do {                                           \
        DO_LOG_FN("trace", ANSI_CYAN, __VA_ARGS__) \
    } while (0)
#else
#define trace(...)
#endif

#if LOG_VERBOSITY >= 5
#define debug(...)                                 \
    do {                                           \
        DO_LOG_FN("debug", ANSI_BLUE, __VA_ARGS__) \
    } while (0)
#else
#define debug(...)
#endif

#endif
