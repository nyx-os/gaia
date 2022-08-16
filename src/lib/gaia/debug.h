/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file
 * @brief Debugging utilities.
 */

#ifndef LIB_GAIA_DEBUG_H
#define LIB_GAIA_DEBUG_H

#include <gaia/host.h>
#include <stdbool.h>

/*! Log severity level */
typedef enum
{
    LOG_INFO,  /*!< Normal output (green) */
    LOG_TRACE, /*!< Trace output (cyan) */
    LOG_DEBUG, /*!< Debug output (blue) */
    LOG_WARN,  /*!< Warning (yellow) */
    LOG_ERROR, /*!< Error (red) */
    LOG_PANIC, /*!< Panic (red), hangs the system. */
} LogLevel;

/**
 * @brief Log a string to the debug output.
 *
 * @param level Severity level.
 * @param file File name.
 * @param fmt Printf-like format string.
 */
void _log(LogLevel level, const char *file, const char *fmt, ...);

/**
 * @brief Asserts that a condition is true, panics if it is not.
 *
 * @param condition The boolean condition to assert.
 * @param condition_string The condition, stringified.
 * @param file File name.
 * @param line Line number.
 */
void _assert(bool condition, const char *condition_string, const char *file, int line);

/**
 * @def __FILENAME__
 * @brief Macro that expands to the current file name.
 */
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

/**
 * @def log(...)
 * @brief calls _log with LOG_INFO level.
 */
#define log(...) _log(LOG_INFO, __FILENAME__, __VA_ARGS__)

/**
 * @def trace(...)
 * @brief calls _log with LOG_TRACE level.
 */
#define trace(...) _log(LOG_TRACE, __FILENAME__, __VA_ARGS__)

/**
 * @def debug(...)
 * @brief calls _log with LOG_DEBUG level.
 */
#define debug(...) _log(LOG_DEBUG, __FILENAME__, __VA_ARGS__)

/**
 * @def warn(fmt, ...)
 * @brief calls _log with LOG_WARN level.
 */
#define warn(...) _log(LOG_WARN, __FILENAME__, __VA_ARGS__)

/**
 * @def error(...)
 * @brief calls _log with LOG_ERROR level.
 */
#define error(...) _log(LOG_ERROR, __FILENAME__, __VA_ARGS__)

/**
 * @def panic(...)
 * @brief calls _log with LOG_PANIC level.
 */
#define panic(...) _log(LOG_PANIC, __FILENAME__, __VA_ARGS__)

/**
 * @def assert(expr)
 * @brief Panics if expr is false.
 */
#define assert(expr) (expr ? (void)0 : _assert(expr, #expr, __FILE__, __LINE__))

#endif /* LIB_GAIA_DEBUG_H */
