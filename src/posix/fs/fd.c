/* SPDX-License-Identifier: BSD-2-Clause */
#include "posix/stat.h"
#include <posix/errno.h>
#include <posix/posix.h>
#include <posix/fnctl.h>
#include <kern/term/term.h>

int sys_open(task_t *proc, const char *path, int mode)
{
    fd_t *new_file = kmalloc(sizeof(fd_t));
    vnode_t *vn = NULL;
    int r = 0;

    r = vfs_find_and(proc->cwd, &vn, path, NULL, VFS_FIND_OR_ERROR, NULL);

    if (r < 0 && mode & O_CREAT) {
        r = vfs_find_and(proc->cwd, &vn, path, NULL, VFS_FIND_AND_CREATE, NULL);

        if (r < 0)
            return r;
    }

    if (!vn && r < 0) {
        return r;
    }

    if (vn->ops.open) {
        r = VOP_OPEN(vn, mode);

        if (r < 0)
            return r;
    }

    new_file->vnode = vn;
    new_file->position = 0;
    new_file->fd = proc->current_fd++;

    proc->files[new_file->fd] = new_file;

    return new_file->fd;
}

int sys_read(task_t *proc, int fd, void *buf, size_t bytes)
{
    fd_t *file = NULL;
    int r;

    if (fd < 0 || fd > 64) {
        return -EBADF;
    }

    file = proc->files[fd];

    if (!file) {
        return -EBADF;
    }

    r = vfs_read(file->vnode, buf, bytes, file->position);

    if (r < 0)
        return r;

    file->position += r;

    return r;
}

int sys_write(task_t *proc, int fd, void *buf, size_t bytes)
{
    fd_t *file = NULL;
    int r;

    if (fd < 0 || fd > 64) {
        return -EBADF;
    }

    file = proc->files[fd];
    if (!file) {
        return -EBADF;
    }

    r = vfs_write(file->vnode, buf, bytes, file->position);

    if (r < 0)
        return r;

    file->position += r;

    return r;
}

int sys_seek(task_t *proc, int fd, off_t offset, int whence)
{
    fd_t *file = NULL;
    int r;

    file = proc->files[fd];

    if (!file) {
        return -EBADF;
    }

    switch (whence) {
    case SEEK_SET:
        file->position = offset;
        break;
    case SEEK_CUR:
        file->position += offset;
        break;
    case SEEK_END: {
        vattr_t attr;
        r = VOP_GETATTR(file->vnode, &attr);

        if (r < 0)
            return -1;
        file->position = attr.size + offset;
        break;
    }
    }

    return file->position;
}

int sys_close(task_t *proc, int fd)
{
    if (fd < 3) {
        return 0;
    }

    fd_t *file = NULL;

    file = proc->files[fd];

    if (!file) {
        return -EBADF;
    }

    kfree(file, sizeof(fd_t));

    return 0;
}

int sys_ioctl(task_t *proc, int fd, int request, void *out)
{
    fd_t *file = NULL;

    if (fd < 0 || fd > 64) {
        return -EBADF;
    }

    file = proc->files[fd];

    if (!file) {
        return -EBADF;
    }

    int r = VOP_IOCTL(file->vnode, request, out);

    if (r < 0)
        return r;

    return 0;
}

int sys_stat(task_t *proc, int fd, const char *path, int flags,
             struct stat *out)
{
    int r;
    vnode_t *vn;
    vattr_t attr;

    (void)flags;

    if (fd == AT_FDCWD) {
        r = vfs_find_and(proc->cwd, &vn, path, NULL, VFS_FIND_OR_ERROR, NULL);

        if (r < 0)
            return r;
    }

    else {
        fd_t *file = proc->files[fd];

        if (!file)
            return -EBADF;

        if (path && strlen(path) > 0) {
            r = vfs_find_and(file->vnode, &vn, path, NULL, VFS_FIND_OR_ERROR,
                             NULL);

            if (r < 0)
                return r;
        } else {
            vn = file->vnode;
        }
    }

    if (vn->ops.getattr)
        r = vn->ops.getattr(vn, &attr);
    if (r < 0)
        return r;

    *out = (struct stat){ 0 };

    out->st_mode = attr.mode;

    out->st_mtime = attr.time;
    out->st_atime = attr.time;

    switch (attr.type) {
    case VREG:
        out->st_mode |= S_IFREG;
        break;
    case VDIR:
        out->st_mode |= S_IFDIR;
        break;
    case VLNK:
        out->st_mode |= S_IFLNK;
        break;
    case VCHR:
        out->st_mode |= S_IFCHR;
        break;

    default:
        panic("Unknown attr: %d", attr.type);
        break;
    }

    out->st_blksize = 512;
    out->st_size = attr.size;
    out->st_blocks = attr.size / 512;

    return 0;
}

int sys_readdir(task_t *proc, int fd, void *buf, size_t max_size,
                size_t *bytes_read)
{
    fd_t *file = NULL;
    int r;
    vattr_t attr;

    file = proc->files[fd];

    if (!file)
        return -EBADF;

    r = VOP_GETATTR(file->vnode, &attr);

    if (r < 0)
        return r;

    if (attr.type != VDIR)
        return -ENOTDIR;

    r = VOP_READDIR(file->vnode, buf, max_size, bytes_read, file->position);

    if (r < 0) {
        log("readdir failed");
        return -1;
    }

    file->position = r;

    return 0;
}
