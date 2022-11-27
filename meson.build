project('gaia', ['c', 'nasm'], default_options: ['c_std=c99', 'werror=true', 'buildtype=debug', 'warning_level=3'])

kernel_srcs = []
kernel_inc_dirs = []

subdir('src')

c_args = ['-ffreestanding', '-nostdlib']

arch_path = join_paths(meson.current_source_dir() , 'src/gaia/arch', target_machine.cpu_family())

ld_file = files(arch_path + '/link.ld')[0]

executable('gaia.elf', 
sources: kernel_srcs,
include_directories: kernel_inc_dirs, 
c_args: c_args,
link_args: '@0@'.format(ld_file))