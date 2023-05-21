project('gaia', ['c', 'nasm'], default_options: ['c_std=c11', 'werror=true', 'buildtype=debug', 'warning_level=3'])

kernel_srcs = []
kernel_inc_dirs = []
kernel_deps = []

c_args = ['-ffreestanding', '-nostdlib']


if get_option('debug_trace') == true
    c_args += ['-DDEBUG']
endif

subdir('src')

arch_path = join_paths(meson.current_source_dir() , 'src/arch', target_machine.cpu_family())

ld_file = files(arch_path + '/link.ld')[0]

git = find_program('git', disabler: true, required: false)

if git.found()
  r = run_command('git', 'rev-parse', '--short', 'HEAD', check: false)

  git_rev = ''

  if r.returncode() == 0
    git_rev = r.stdout().strip()
  else 
    git_rev = 'unknown'
  endif

  c_args += ['-D__GAIA_GIT_VERSION__="@0@"'.format(git_rev)]
endif



executable('gaia.elf', 
sources: kernel_srcs,
include_directories: kernel_inc_dirs, 
c_args: c_args,
link_args: '-Wl,-T@0@'.format(ld_file),
dependencies: kernel_deps)
