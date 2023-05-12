#ifndef POSIX_H
#define POSIX_H
#include <kern/sched.h>
#include <posix/fnctl.h>
#include <posix/stat.h>

int sys_open(task_t *proc, const char *path, int mode);
int sys_read(task_t *proc, int fd, void *buf, size_t bytes);
int sys_write(task_t *proc, int fd, void *buf, size_t bytes);
int sys_seek(task_t *proc, int fd, off_t offset, int whence);
int sys_readdir(task_t *proc, int fd, void *buf, size_t max_size,
                size_t *bytes_read);
int sys_close(task_t *proc, int fd);
int sys_stat(task_t *proc, int fd, const char *path, struct stat *out);
int sys_execve(task_t *proc, const char *path, char const *argv[],
               char const *envp[]);

void posix_init(charon_t charon);

#endif
