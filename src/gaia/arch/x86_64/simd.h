/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/** @file
 *  @brief SIMD (Single instruction, multiple data) definitions.
 */

#ifndef ARCH_X86_64_FPU_H
#define ARCH_X86_64_FPU_H

/**
 * @brief initialize the fpu
 *
 */
void simd_initialize(void);

/*
 * @brief save the fpu state
 */
void simd_save_state(void *state);

/*
 * @brief restore the fpu state
 */
void simd_restore_state(void *state);

/*
 * @brief Initialize a fpu context
 */
void simd_initialize_context(void *state);

#endif /* ARCH_X86_64_FPU_H */
