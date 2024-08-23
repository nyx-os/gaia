add_languages('nasm')

kernel_srcs += files('hal.cpp',
 'entry.cpp',
 'gdt.cpp', 
 'idt.cpp',
 'int.cpp',
 'locore.asm',
 'syscall.asm',
 'syscall.cpp',
 'timer.cpp', 
 'hpet.cpp',
 'mmu.cpp',
 'apic.cpp',
 'cpu.cpp',
 'simd.cpp')


cpp_args += [
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

linker_script = join_paths(meson.current_source_dir(), 'link.ld')
