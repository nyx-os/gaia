#include <posix/fs/tar.h>
#include <posix/fs/tmpfs.h>
#include <kern/charon.h>
#include <posix/fs/devfs.h>
#include <posix/tty.h>
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

    VOP_MKDIR(root_vnode, &root_devnode, "dev", NULL);

    tty_init();

    task_t *task = sched_new_task(1, true);

    sys_open(task, "/dev/tty", O_RDWR);
    sys_open(task, "/dev/tty", O_RDWR);
    sys_open(task, "/dev/tty", O_RDWR);

    const char *argv[] = { "/usr/bin/init", NULL };
    const char *envp[] = { "SHELL=/usr/bin/bash", NULL };

    sys_execve(task, "/usr/bin/init", argv, envp);

    return;
}
