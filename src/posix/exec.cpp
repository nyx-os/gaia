#include <elf.h>
#include <kernel/task.hpp>
#include <posix/exec.hpp>

namespace Gaia::Posix {

Result<Void, Error> execve(Task *task, frg::string_view path,
                           char const *argv[], char const *envp[]) {

  auto vnode = TRY(Fs::vfs_find(path.data(), task->cwd));

  (void)vnode;
  (void)argv;
  (void)envp;
  log("Found file {}", path);

  return Ok({});
}

} // namespace Gaia::Posix
