/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_ASM_H_
#define SRC_ARCH_X86_64_ASM_H_
#include <libkern/base.h>

static inline void outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1" ::"a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outw(uint16_t port, uint16_t data)
{
    asm volatile("outw %0, %1" ::"a"(data), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t data;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outl(uint16_t port, uint32_t data)
{
    asm volatile("outl %0, %1" ::"a"(data), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t data;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void cli(void)
{
    asm volatile("cli");
}

static inline void hlt(void)
{
    asm volatile("hlt");
}

static inline void sti(void)
{
    asm volatile("sti");
}

#define ASM_MAKE_CRN(N)                                   \
    static inline uint64_t read_cr##N(void)               \
    {                                                     \
        uint64_t value = 0;                               \
        asm volatile("mov %%cr" #N ", %0" : "=r"(value)); \
        return value;                                     \
    }                                                     \
                                                          \
    static inline void write_cr##N(uint64_t value)        \
    {                                                     \
        asm volatile("mov %0, %%cr" #N ::"a"(value));     \
    }

ASM_MAKE_CRN(0)
ASM_MAKE_CRN(1)
ASM_MAKE_CRN(2)
ASM_MAKE_CRN(3)
ASM_MAKE_CRN(4)

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t edx = 0, eax = 0;
    asm volatile("rdmsr\n\t" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
    return ((uint64_t)edx << 32) | eax;
}

static inline uint64_t wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t eax = (uint32_t)val;
    uint32_t edx = (uint32_t)(val >> 32);
    asm volatile("wrmsr\n\t" : : "a"(eax), "d"(edx), "c"(msr) : "memory");
    return ((uint64_t)edx << 32) | eax;
}
#endif
