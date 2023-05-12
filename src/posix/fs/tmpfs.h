#ifndef POSIX_FS_TMPFS_H
#define POSIX_FS_TMPFS_H
#include <posix/fs/vfs.h>
#include <sys/queue.h>

struct tmpnode;

typedef struct tmp_dirent {
    LIST_HEAD(, tmp_dirent) entries;
    LIST_ENTRY(tmp_dirent) link;

    const char name[256];
    struct tmpnode *node;
} tmp_dirent_t;

typedef struct tmpnode {
    vnode_t *vnode;
    vattr_t attr;

    union {
        /* VDIR */
        struct {
            LIST_HEAD(, tmp_dirent) entries;
            struct tmpnode *parent;
        } dir;

        /* VREG */
        struct {
            void *buffer;
        } reg;

        /* VLNK */
        struct {
            char *to_name;
            struct tmpnode *to;
        } link;
    } data;

} tmp_node_t;

void tmpfs_init(void);

#endif
