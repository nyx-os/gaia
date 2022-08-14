/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file
 * @brief Host architecture specific definitions.
 */

#ifndef LIB_GAIA_HOST_H
#define LIB_GAIA_HOST_H

/**
 * @brief Write a string to the debug output.
 *
 * This function is used for writing strings in the debugging functions such as log, panic and assert.
 *
 * @param str String to write.
 */
void host_debug_write_string(const char *str);

/**
 * @brief Freeze the system.
 *
 * This function hangs the host system, making further code execution impossible.
 *
 * @return This function should NOT return.
 */
void host_hang();

#endif /* GAIA_ARCH_HOST_H */