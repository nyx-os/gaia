kernel_inc_dirs += include_directories('.')
subdir(target_machine.cpu_family())

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