/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef POSIX_FS_DEVFS_H
#define POSIX_FS_DEVFS_H
#include <sys/queue.h>
#include <posix/types.h>
#include <stddef.h>

struct vnode;

typedef struct devnode {
    dev_t dev;
    LIST_ENTRY(devnode) link;
} devnode_t;

void devfs_setup_vnode(struct vnode *vn, dev_t dev);

int devfs_read(struct vnode *vn, void *buf, size_t nbyte, off_t off);
int devfs_write(struct vnode *vn, void *buf, size_t nbyte, off_t off);

#endif
