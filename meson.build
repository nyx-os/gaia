project(
  'gaia',
  ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'build.cpp_std=c++20',
    'werror=false',
    'buildtype=debug',
    'warning_level=2',
    'b_sanitize=undefined',
  ],
)

test_srcs = []
kernel_srcs = []
kernel_inc = []
cpp_args = ['-ffreestanding', '-nostdlib', '-D__KERNEL__']
kernel_deps = []
test_deps = []

#add_global_arguments('-DFLANTERM_FB_DISABLE_BUMP_ALLOC', language: 'c')

subdir('src')

# Kernel deps
kernel_deps += dependency(
  'frigg',
  default_options: ['frigg_no_install=true'],
  fallback: ['frigg', 'frigg_dep'],
)

kernel_deps += dependency('chdrs', fallback: ['chdrs', 'chdrs_dep'])
kernel_deps += dependency('cxxhdrs', fallback: ['cxxhdrs', 'cxxhdrs_dep'])
kernel_deps += dependency('lai', fallback: ['lai', 'dependency'])
kernel_deps += dependency('flanterm', fallback: ['flanterm', 'flanterm_dep'])

test_deps += dependency(
  'frigg',
  default_options: ['frigg_no_install=true'],
  fallback: ['frigg', 'frigg_dep'],
  native: true,
)

if meson.is_cross_build()
  subdir('tests')
endif


git = find_program('git', disabler: true, required: false)

if git.found()
  r = run_command('git', 'rev-parse', '--short', 'HEAD', check: false)

  git_rev = ''

  if r.returncode() == 0
    git_rev = r.stdout().strip()
  else
    git_rev = 'unknown'
  endif

  cpp_args += ['-D__GAIA_GIT_VERSION__="@0@"'.format(git_rev)]
endif

executable(
  'gaia.elf',
  sources: kernel_srcs,
  include_directories: kernel_inc,
  cpp_args: cpp_args,
  c_args: cpp_args,
  dependencies: kernel_deps,
  link_args: '-Wl,-T@0@'.format(linker_script),
)
