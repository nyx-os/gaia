#ifndef ARCH_X86_64_ASM_H
#define ARCH_X86_64_ASM_H
#include <stdint.h>

/* Modified version of  https://github.com/brutal-org/brutal/blob/main/sources/kernel/x86_64/asm.h, all credit goes to the author */

#define ASM_MAKE_CRN(N)                                   \
    static inline uint64_t asm_read_cr##N(void)           \
    {                                                     \
        uint64_t value = 0;                               \
        __asm__ volatile("mov %%cr" #N ", %0"             \
                         : "=r"(value));                  \
        return value;                                     \
    }                                                     \
                                                          \
    static inline void asm_write_cr##N(uint64_t value)    \
    {                                                     \
        __asm__ volatile("mov %0, %%cr" #N ::"a"(value)); \
    }

ASM_MAKE_CRN(0)
ASM_MAKE_CRN(1)
ASM_MAKE_CRN(2)
ASM_MAKE_CRN(3)
ASM_MAKE_CRN(4)

static inline uint64_t asm_read_xcr(uint32_t i)
{
    uint32_t eax, edx;
    __asm__ volatile("xgetbv"

                     : "=a"(eax), "=d"(edx)
                     : "c"(i)
                     : "memory");

    return eax | ((uint64_t)edx << 32);
}

static inline void asm_write_xcr(uint32_t i, uint64_t value)
{
    uint32_t edx = value >> 32;
    uint32_t eax = (uint32_t)value;
    __asm__ volatile("xsetbv"
                     :
                     : "a"(eax), "d"(edx), "c"(i)
                     : "memory");
}

static inline void asm_xsave(uint8_t *region)
{
    __asm__ volatile("xsave %0" ::"m"(*region), "a"(~(uintptr_t)0), "d"(~(uintptr_t)0)
                     : "memory");
}

static inline void asm_xrstor(uint8_t *region)
{
    __asm__ volatile("xrstor %0" ::"m"(*region), "a"(~(uintptr_t)0), "d"(~(uintptr_t)0)
                     : "memory");
}

static inline void asm_fninit(void)
{
    __asm__ volatile("fninit");
}

static inline void asm_fxsave(void *region)
{
    __asm__ volatile("fxsave (%0)" ::"a"(region));
}

static inline void asm_fxrstor(void *region)
{
    __asm__ volatile("fxrstor (%0)" ::"a"(region));
}

enum cr4_bit
{
    CR4_VIRTUAL_8086_MODE_EXT = (1 << 0),
    CR4_PROTECTED_MODE_VIRTUAL_INT = (1 << 1),
    CR4_TIME_STAMP_DISABLE = (1 << 2),
    CR4_DEBUGGING_EXT = (1 << 3),
    CR4_PAGE_SIZE_EXT = (1 << 4),
    CR4_PHYSICAL_ADDRESS_EXT = (1 << 5),
    CR4_MACHINE_CHECK_EXCEPTION = (1 << 6),
    CR4_PAGE_GLOBAL_ENABLE = (1 << 7),
    CR4_PERFORMANCE_COUNTER_ENABLE = (1 << 8),
    CR4_FXSR_ENABLE = (1 << 9),
    CR4_SIMD_EXCEPTION_SUPPORT = (1 << 10),
    CR4_USER_MODE_INSTRUCTION_PREVENTION = (1 << 11),
    CR4_5_LEVEL_PAGING_ENABLE = (1 << 12),
    CR4_VIRTUAL_MACHINE_EXT_ENABLE = (1 << 13),
    CR4_SAFER_MODE_EXT_ENABLE = (1 << 14),
    CR4_FS_GS_BASE_ENABLE = (1 << 16),
    CR4_PCID_ENABLE = (1 << 17),
    CR4_XSAVE_ENABLE = (1 << 18),
    CR4_SUPERVISOR_EXE_PROTECTION_ENABLE = (1 << 20),
    CR4_SUPERVISOR_ACCESS_PROTECTION_ENABLE = (1 << 21),
    CR4_KEY_PROTECTION_ENABLE = (1 << 22),
    CR4_CONTROL_FLOW_ENABLE = (1 << 23),
    CR4_SUPERVISOR_KEY_PROTECTION_ENABLE = (1 << 24),
};

enum xcr0_bit
{
    XCR0_XSAVE_SAVE_X87 = (1 << 0),
    XCR0_XSAVE_SAVE_SSE = (1 << 1),
    XCR0_AVX_ENABLE = (1 << 2),
    XCR0_BNDREG_ENABLE = (1 << 3),
    XCR0_BNDCSR_ENABLE = (1 << 4),
    XCR0_AVX512_ENABLE = (1 << 5),
    XCR0_ZMM0_15_ENABLE = (1 << 6),
    XCR0_ZMM16_32_ENABLE = (1 << 7),
    XCR0_PKRU_ENABLE = (1 << 9),
};

enum cr0_bit
{
    CR0_PROTECTED_MODE_ENABLE = (1 << 0),
    CR0_MONITOR_CO_PROCESSOR = (1 << 1),
    CR0_EMULATION = (1 << 2),
    CR0_TASK_SWITCHED = (1 << 3),
    CR0_EXTENSION_TYPE = (1 << 4),
    CR0_NUMERIC_ERROR_ENABLE = (1 << 5),
    CR0_WRITE_PROTECT_ENABLE = (1 << 16),
    CR0_ALIGNMENT_MASK = (1 << 18),
    CR0_NOT_WRITE_THROUGH_ENABLE = (1 << 29),
    CR0_CACHE_DISABLE = (1 << 30),
    CR0_PAGING_ENABLE = (1 << 31),
};

#endif /* ARCH_X86_64_ASM_H */
