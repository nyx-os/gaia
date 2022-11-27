project('gaia', ['c', 'nasm'], default_options: ['c_std=c99', 'werror=true', 'buildtype=debug', 'warning_level=3'])

kernel_srcs = []
kernel_inc_dirs = []
kernel_deps = []

subdir('src')

c_args = ['-ffreestanding', '-nostdlib']

arch_path = join_paths(meson.current_source_dir() , 'src/gaia/arch', target_machine.cpu_family())

ld_file = files(arch_path + '/link.ld')[0]

stdc = subproject('stdc-shim')

kernel_deps += stdc.get_variable('dep')

executable('gaia.elf', 
sources: kernel_srcs,
include_directories: kernel_inc_dirs, 
c_args: c_args,
link_args: '-Wl,-T@0@'.format(ld_file),
dependencies: kernel_deps)
