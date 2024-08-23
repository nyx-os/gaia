test_srcs += files(
    'elf.cpp',
    'freelist.cpp',
    'list.cpp',
    'main.cpp',
    'path.cpp',
    'ringbuffer.cpp',
)
test_deps += dependency('catch2', native: true)
test(
    'all_tests',
    executable(
        'tests',
        test_srcs,
        cpp_args: ['-fsanitize=address', '--coverage'],
        link_args: ['-fsanitize=address', '--coverage'],
        include_directories: kernel_inc,
        dependencies: test_deps,
        native: true,
    ),
)