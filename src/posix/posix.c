#include <posix/fs/tar.h>
#include <posix/fs/tmpfs.h>
#include <kern/charon.h>
#include <posix/posix.h>

void posix_init(charon_t charon)
{
    void *ramdisk = NULL;

    for (size_t i = 0; i < charon.modules.count; i++) {
        const char *name = charon.modules.modules[i].name;

        if (strncmp(name, "/ramdisk.tar", strlen(name)) == 0) {
            ramdisk = (void *)charon.modules.modules[i].address;
            break;
        }
    }

    assert(ramdisk != NULL);

    tmpfs_init();

    tar_write_on_tmpfs(ramdisk);

    task_t *task = sched_new_task(69, true);

    const char *argv[] = { "/usr/bin/bash", NULL };
    const char *envp[] = { "HOME=/", NULL };

    sys_execve(task, "/usr/bin/bash", argv, envp);

    return;
}
