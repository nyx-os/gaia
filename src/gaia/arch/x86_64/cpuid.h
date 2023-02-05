/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef ARCH_X86_64_CPUID_H
#define ARCH_X86_64_CPUID_H
#include <gaia/base.h>

typedef struct
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} Cpuid;

Cpuid cpuid(uint32_t leaf, uint32_t subleaf);

#define CPUID_LEAF_EXTENDED_FEATURES 0x80000001
#define CPUID_PROC_EXTENDED_STATE_ENUMERATION 13

// Stored in ECX
typedef enum
{
    CPUID_EXFEATURE_FPU = 1 << 0,
    CPUID_EXFEATURE_VME = 1 << 1,
    CPUID_EXFEATURE_DE = 1 << 2,
    CPUID_EXFEATURE_PSE = 1 << 3,
    CPUID_EXFEATURE_TSC = 1 << 4,
    CPUID_EXFEATURE_MSR = 1 << 5,
    CPUID_EXFEATURE_PAE = 1 << 6,
    CPUID_EXFEATURE_MCE = 1 << 7,
    CPUID_EXFEATURE_CX8 = 1 << 8,
    CPUID_EXFEATURE_APIC = 1 << 9,
    CPUID_EXFEATURE_SYSCALL = 1 << 11,
    CPUID_EXFEATURE_MTRR = 1 << 12,
    CPUID_EXFEATURE_PGE = 1 << 13,
    CPUID_EXFEATURE_MCA = 1 << 14,
    CPUID_EXFEATURE_CMOV = 1 << 15,
    CPUID_EXFEATURE_PAT = 1 << 16,
    CPUID_EXFEATURE_PSE36 = 1 << 17,
    CPUID_EXFEATURE_MP = 1 << 19,
    CPUID_EXFEATURE_NX = 1 << 20,
    CPUID_EXFEATURE_MMXEXT = 1 << 22,
    CPUID_EXFEATURE_MMX = 1 << 23,
    CPUID_EXFEATURE_FXSR = 1 << 24,
    CPUID_EXFEATURE_FXSR_OPT = 1 << 25,
    CPUID_EXFEATURE_PDPE1GB = 1 << 26,
    CPUID_EXFEATURE_RDTSCP = 1 << 27,
} CpuidExtendedFeatureBits;

bool cpuid_has_feature(Cpuid cpuid, uint32_t feature);

static inline UNUSED bool cpuid_supports_1gb_pages(void)
{
    return cpuid_has_feature(cpuid(CPUID_LEAF_EXTENDED_FEATURES, 0), CPUID_EXFEATURE_PDPE1GB);
}

#endif /* ARCH_X86_64_CPUID_H */
