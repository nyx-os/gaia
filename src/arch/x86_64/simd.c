#include <libkern/base.h>
#include <asm.h>
#include <cpuid.h>
#include <kern/vm/phys.h>

static void (*simd_save)(void *) = NULL;
static void (*simd_restore)(void *) = NULL;
static size_t simd_buffer_size = 0;
static uint8_t *initial_context = NULL;

#define CPUID_XSAVE ((uint32_t)1 << 26)
#define CPUID_AVX ((uint32_t)1 << 28)
#define CPUID_AVX512 ((uint32_t)1 << 16)

static void simd_xsave(void *buffer)
{
    xsave(buffer);
}

static void simd_xrstor(void *buffer)
{
    xrstor(buffer);
}

static void simd_fxsave(void *buffer)
{
    fxsave(buffer);
}

static void simd_fxrstor(void *buffer)
{
    fxrstor(buffer);
}

void simd_init(void)
{
    cpuid_t cpu = cpuid(CPUID_EXFEATURE_FPU, 0);
    uint64_t cr0 = read_cr0();

    cr0 &= ~((uint64_t)CR0_EMULATION);
    cr0 |= CR0_MONITOR_CO_PROCESSOR;
    cr0 |= CR0_NUMERIC_ERROR_ENABLE;

    write_cr0(cr0);

    write_cr4(read_cr4() | CR4_FXSR_ENABLE);

    write_cr4(read_cr4() | CR4_SIMD_EXCEPTION_SUPPORT);

    if ((cpu.ecx & CPUID_XSAVE) == CPUID_XSAVE) {
        log("CPU supports xsave");
        write_cr4(read_cr4() | CR4_XSAVE_ENABLE);

        uint64_t xcr0 = 0;
        xcr0 |= XCR0_XSAVE_SAVE_X87;
        xcr0 |= XCR0_XSAVE_SAVE_SSE;
        write_xcr(0, xcr0);

        cpuid_t cpu = cpuid(CPUID_PROC_EXTENDED_STATE_ENUMERATION, 0);
        simd_save = simd_xsave;
        simd_restore = simd_xrstor;
        simd_buffer_size = cpu.ecx;
    } else {
        log("Using legacy fxsave");
        simd_save = simd_fxsave;
        simd_restore = simd_fxrstor;
        simd_buffer_size = 512;
    }

    fninit();

    initial_context = (void *)P2V(phys_allocz());
    simd_save(initial_context);
}

void simd_save_state(void *state)
{
    simd_save(state);
}

void simd_restore_state(void *state)
{
    simd_restore(state);
}

void simd_init_context(void *state)
{
    memcpy(state, initial_context, simd_buffer_size);
}
