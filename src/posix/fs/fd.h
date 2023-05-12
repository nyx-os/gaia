#ifndef POSIX_FS_FD_H
#define POSIX_FS_FD_H
#include <posix/fs/vfs.h>
#include <posix/stat.h>

struct fd;

typedef struct {
    int (*read)(struct fd *file, void *buf, size_t bytes, off_t offset);
    int (*write)(struct fd *file, void *buf, size_t bytes, off_t offset);
} fd_ops_t;

typedef struct fd {
    int fd;
    vnode_t *vnode;
    size_t position;
    fd_ops_t ops;
} fd_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif
