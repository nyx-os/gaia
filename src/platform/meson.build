kernel_inc += include_directories('.')

target = target_machine.cpu_family()

if target == 'x86_64'
  subdir('amd64')
else
  subdir(target)
endif
