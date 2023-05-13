/* SPDX-License-Identifier: BSD-2-Clause */
#include <posix/fs/tar.h>
#include <posix/fs/vfs.h>
#include <posix/stat.h>
#include <posix/errno.h>
#include <libkern/base.h>

static inline uint64_t oct2int(const char *str, size_t len)
{
    uint64_t value = 0;
    while (*str && len > 0) {
        value = value * 8 + (*str++ - '0');
        len--;
    }
    return value;
}

void tar_write_on_tmpfs(void *archive)
{
    tar_header_t *current_file = (tar_header_t *)archive;

    while (strncmp(current_file->magic, "ustar", 5) == 0) {
        char *name = current_file->name;
        vattr_t attr = { 0 };

        attr.mode =
                oct2int(current_file->mode, sizeof(current_file->mode) - 1) &
                ~(S_IFMT);

        uint64_t size = oct2int(current_file->size, sizeof(current_file->size));

        switch (current_file->type) {
        case TAR_NORMAL_FILE: {
            int ret = 0;
            vnode_t *vn;

            ret = vfs_find_and(NULL, &vn, name, NULL, VFS_FIND_AND_CREATE,
                               &attr);

            if (ret < 0) {
                panic("Failed making file %s, error is %s", name,
                      ret == -ENOTDIR ? "enodir" : "dunno");
            }

            vfs_write(vn, (uint8_t *)current_file + 512, size, 0);

            break;
        }

        case TAR_DIRECTORY: {
            int ret = 0;
            vnode_t *vn;

            ret = vfs_find_and(NULL, &vn, name, NULL, VFS_FIND_AND_MKDIR,
                               &attr);

            if (ret < 0) {
                panic("Failed making directory %s", name);
            }
            break;
        }

        case TAR_SYMLINK: {
            int ret = 0;
            vnode_t *vn;

            // log("%s: symlink to %s", name, (char *)current_file->link_name);

            ret = vfs_find_and(NULL, &vn, name, current_file->link_name,
                               VFS_FIND_AND_LINK, &attr);

            if (ret < 0) {
                panic("Failed linking %s to %s", name, current_file->link_name);
            }

            break;
        }
        }

        current_file = (tar_header_t *)((uint8_t *)current_file + 512 +
                                        ALIGN_UP(size, 512));
    }
}
