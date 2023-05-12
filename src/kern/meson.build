kernel_srcs += files('main.c', 'sched.c', 'syscall.c')

if get_option('sanitizers') == true
    subdir('san')
    c_args += ['-fsanitize=undefined']
endif

subdir('vm')
subdir('term')
