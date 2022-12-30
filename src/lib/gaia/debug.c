/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include <gaia/base.h>
#include <gaia/debug.h>
#include <stdarg.h>
#include <stdc-shim/stdlib.h>
#include <stdc-shim/string.h>

static void vstream_printf(void (*callback)(const char *s), const char *fmt, va_list args)
{
    const char *s = fmt;
    while (*s)
    {
        if (*s == '%')
        {
            s++;
            switch (*s)
            {
            case 'd':
            {
                char buf[100];
                int64_t value = va_arg(args, int64_t);
                callback(itoa(value, buf, 10));
                break;
            }
            case 'u':
            {
                char buf[100];
                uint64_t value = va_arg(args, uint64_t);
                callback(utoa(value, buf, 10));
                break;
            }
            case 'p':
            {
                char buf[100];
                uint64_t value = va_arg(args, uint64_t);
                callback(utoa(value, buf, 16));
                break;
            }

            case 'x':
            {
                char buf[100];
                int64_t value = va_arg(args, int64_t);
                callback(utoa(value, buf, 16));
                break;
            }

            case 's':
            {
                const char *value = va_arg(args, const char *);
                callback(value);
                break;
            }
            case 'c':
            {
                char value = va_arg(args, int);
                callback(&value);
                break;
            }
            case '%':
            {
                callback("%");
                break;
            }
            default:
            {
                break;
            }
            }
        }
        else
        {
            char tmp[] = {*s, 0};
            callback(tmp);
        }
        s++;
    }
}

static void stream_printf(void (*callback)(const char *s), const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vstream_printf(callback, fmt, args);
    va_end(args);
}

void _log(LogLevel level, const char *file, const char *fmt, ...)
{
    switch (level)
    {
    case LOG_NONE:
        break;
    case LOG_INFO:
        stream_printf(host_debug_write_string, "\x1b[32m");
        break;
    case LOG_WARN:
        stream_printf(host_debug_write_string, "\x1b[33m");
        break;
    case LOG_TRACE:
        stream_printf(host_debug_write_string, "\x1b[36m");
        break;
    case LOG_DEBUG:
        stream_printf(host_debug_write_string, "\x1b[34m");
        break;
    case LOG_PANIC:
    case LOG_ERROR:
        stream_printf(host_debug_write_string, "\x1b[31m");
        break;
    }

    if (level != LOG_NONE)
    {
        char file_name[100] = {0};
        host_accelerated_copy(file_name, file, strlen(file));
        file_name[strlen(file) - 2] = 0;
        stream_printf(host_debug_write_string, "%s:\x1b[0m ", file_name);
    }

    va_list args;
    va_start(args, fmt);
    vstream_printf(host_debug_write_string, fmt, args);
    va_end(args);

    if (level != LOG_NONE)
        host_debug_write_string("\n");

    if (level == LOG_PANIC)
    {
        host_hang();
    }
}

void _assert(bool condition, const char *condition_string, const char *file, int line)
{
    if (!condition)
    {
        _log(LOG_PANIC, file, "Assertion failed: %s, at %s:%d", condition_string, file, line);
    }
    else
    {
        __builtin_unreachable();
    }
}
