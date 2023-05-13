/* SPDX-License-Identifier: BSD-2-Clause */
#include <posix/fs/vfs.h>
#include <libkern/base.h>

vnode_t *root_vnode = NULL;
vnode_t *root_devnode = NULL;

// Took inspiration from https://github.com/NetaScale/SCAL-UX/blob/old-22-08-07/Kernel/posix/vfs.c#L79, licensed under the MPL-2.0 License
/* Copyright 2022 NetaScale Systems Ltd.
 * All rights reserved.
 */

int vfs_find_and(vnode_t *cwd, vnode_t **out, const char *pathname,
                 const char *link, int flags, vattr_t *attr)
{
    vnode_t *vn;
    char path[255];
    char *next, *sub;
    size_t sublen = 0;
    size_t len = strlen(pathname);
    int result = 0;
    bool last = false;

    if (len > 255) {
        panic("length is greater than 255");
    }

    if (pathname[0] == '/' || !cwd) {
        vn = root_vnode;

        // The path is just '/'
        if (!pathname[1]) {
            *out = vn;
            return 0;
        }
    } else {
        vn = cwd;
    }

    strncpy(path, pathname, len);

    sub = path;

    // If the path ends with /, we can just remove the slashes
    if (path[len - 1] == '/') {
        // Accept paths ending with multiple slashes like /home/user////////
        size_t i = len - 1;
        while (path[i] == '/') {
            path[i--] = '\0';
        }

        // if the path is just slashes
        if (!*path) {
            *out = vn;
            return 0;
        }
    }

    while (true) {
        sublen = 0;
        next = sub;

        while (*next != '/' && *next) {
            next++;
            sublen++;
        }

        if (!*next)
            last = true;
        else
            *next = '\0';

        if (sublen == 0) {
            if (last)
                break;
            else {
                sub += sublen + 1;
                continue;
            }
        }

        // If this isn't the last subpath or it's a file that already exists
        if (!last ||
            (!(flags & VFS_FIND_AND_CREATE) && !(flags & VFS_FIND_AND_MKDIR) &&
             !(flags & VFS_FIND_AND_LINK))) {
            result = VOP_LOOKUP(vn, &vn, sub);
        } else if (flags & VFS_FIND_AND_CREATE) {
            result = VOP_CREATE(vn, &vn, sub, attr);
        } else if (flags & VFS_FIND_AND_MKDIR) {
            result = VOP_MKDIR(vn, &vn, sub, attr);
        } else if (flags & VFS_FIND_AND_LINK) {
            result = VOP_LINK(vn, &vn, sub, link, attr);
        }

        if (result < 0) {
            return result;
        }

        if (last)
            break;

        sub += sublen + 1;
    }

    *out = vn;
    return 0;
}

int vfs_read(vnode_t *vn, void *buf, size_t nbyte, size_t off)
{
    return VOP_READ(vn, buf, nbyte, off);
}

int vfs_write(vnode_t *vn, void *buf, size_t nbyte, size_t off)
{
    return VOP_WRITE(vn, buf, nbyte, off);
}

int vfs_getdents(vnode_t *vn, void *buf, size_t max_size, size_t *bytes_read)
{
    return VOP_READDIR(vn, buf, max_size, bytes_read);
}

int vfs_mkdir(vnode_t *vn, vnode_t **out, const char *name, vattr_t *vattr)
{
    return VOP_MKDIR(vn, out, name, vattr);
}

int vfs_create(vnode_t *vn, vnode_t **out, const char *name, vattr_t *vattr)
{
    return VOP_CREATE(vn, out, name, vattr);
}
