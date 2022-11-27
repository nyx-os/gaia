kernel_srcs += files('debug.c', 'slab.c', 'spinlock.c', 'vec.c')

if get_option('kubsan') == true
    kernel_srcs += files('tinyubsan.c')
endif