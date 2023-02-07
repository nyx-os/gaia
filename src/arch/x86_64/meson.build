kernel_srcs += files(
'entry.c',
'cpu_machdep.c',
'machdep.c',
'vm_machdep.c',
'cpuid.c',
'gdt.c',
'intr_machdep.c',
'hpet.c',
'apic.c',
'locore.asm')

kernel_inc_dirs += include_directories('.')

c_args += [
        '-mabi=sysv',
        '-mno-80387',
        '-mno-mmx',
        '-mno-3dnow',
        '-mno-sse',
        '-mno-sse2',
        '-mno-ssse3',
        '-mno-sse4',
        '-mno-sse4a',
        '-mno-sse4.1',
        '-mno-sse4.2',
        '-mno-avx',
        '-mno-avx2',
        '-mno-avx512f',
        '-mno-red-zone',
        '-mcmodel=kernel']
