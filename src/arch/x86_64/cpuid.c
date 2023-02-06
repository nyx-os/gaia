/* SPDX-License-Identifier: BSD-2-Clause */
#include "cpuid.h"

cpuid_t cpuid(uint32_t leaf, uint32_t subleaf)
{
    uint32_t cpuid_max;

    cpuid_t ret;

    asm volatile("cpuid"
                 : "=a"(cpuid_max)
                 : "a"(leaf & 0x80000000)
                 : "rbx", "rcx", "rdx");

    if (leaf > cpuid_max) {
        return (cpuid_t){ -1, -1, -1, -1 };
    }

    asm volatile("cpuid"
                 : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
                 : "a"(leaf), "c"(subleaf));

    return ret;
}
