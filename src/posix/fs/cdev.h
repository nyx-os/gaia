/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef POSIX_FS_CDEV_H
#define POSIX_FS_CDEV_H
#include <posix/fs/vfs.h>
#include <libkern/base.h>

typedef struct cdev {
    bool is_atty : 1, present : 1;
    void *data;

    int (*read)(dev_t dev, void *buf, size_t nbyte, size_t off);
    int (*write)(dev_t dev, void *buf, size_t nbyte, size_t off);
    int (*ioctl)(dev_t dev, int req, void *out);
} cdev_t;

/* Allocates a new character device and returns a major number */

int cdev_allocate(cdev_t *dev);

extern vnode_t *devfs_root;
extern cdev_t cdevs[64];

#endif
