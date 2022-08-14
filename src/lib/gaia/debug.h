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

/*! Log severity level */
typedef enum
{
    LOG_INFO,  /*!< Normal output (green) */
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
 * @def log(fmt, ...)
 * @brief calls _log with LOG_INFO level.
 */
#define log(fmt, ...) _log(LOG_INFO, __FILE__, fmt, ##__VA_ARGS__)

/**
 * @def warn(fmt, ...)
 * @brief calls _log with LOG_WARN level.
 */
#define warn(fmt, ...) _log(LOG_WARN, __FILE__, fmt, ##__VA_ARGS__)

/**
 * @def error(fmt, ...)
 * @brief calls _log with LOG_ERROR level.
 */
#define error(fmt, ...) _log(LOG_ERROR, __FILE__, fmt, ##__VA_ARGS__)

/**
 * @def panic(fmt, ...)
 * @brief calls _log with LOG_PANIC level.
 */
#define panic(fmt, ...) _log(LOG_PANIC, __FILE__, fmt, ##__VA_ARGS__)

#endif /* LIB_GAIA_DEBUG_H */
