/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <cpuid.h>

Cpuid cpuid(uint32_t leaf, uint32_t subleaf)
{
    Cpuid ret = {0};

    uint32_t cpuid_max;

    __asm__ volatile("cpuid"
                     : "=a"(cpuid_max)
                     : "a"(leaf & 0x80000000)
                     : "ebx", "ecx", "edx");

    assert(leaf < cpuid_max);

    __asm__ volatile("cpuid"
                     : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
                     : "a"(leaf), "c"(subleaf));
    return ret;
}

bool cpuid_has_feature(Cpuid cpuid, uint32_t feature)
{
    return (cpuid.edx & feature) == feature;
}
