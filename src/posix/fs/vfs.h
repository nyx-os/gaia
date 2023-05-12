#ifndef POSIX_FS_VFS_H
#define POSIX_FS_VFS_H
#include <posix/types.h>
#include <stddef.h>

#define VFS_FIND_OR_ERROR (1 << 0)
#define VFS_FIND_AND_CREATE (1 << 1)
#define VFS_FIND_AND_MKDIR (1 << 2)
#define VFS_FIND_AND_LINK (1 << 3)

typedef enum {
    VNON, /* No type */
    VREG, /* Regular file */
    VDIR, /* Directory */
    VLNK, /* Symlink */
} vnode_type_t;

struct vnode;

typedef struct {
    vnode_type_t type;
    size_t size;
    mode_t mode;
    uid_t uid;
    gid_t gid;
} vattr_t;

typedef struct {
    int (*lookup)(struct vnode *vn, struct vnode **out, const char *name);
    int (*create)(struct vnode *vn, struct vnode **out, const char *name,
                  vattr_t *vattr);
    int (*getattr)(struct vnode *vn, vattr_t *out);
    int (*open)(struct vnode *vn, int mode);
    int (*read)(struct vnode *vn, void *buf, size_t nbyte, size_t off);
    int (*write)(struct vnode *vn, void *buf, size_t nbyte, size_t off);
    int (*mkdir)(struct vnode *vn, struct vnode **out, const char *name,
                 vattr_t *vattr);

    int (*readdir)(struct vnode *vn, void *buf, size_t max_size,
                   size_t *bytes_read);
    int (*close)(struct vnode *vn);

    int (*symlink)(struct vnode *vn, struct vnode **out, const char *path,
                   const char *link, vattr_t *attr);
} vnode_ops_t;

typedef struct vnode {
    vnode_type_t type;
    void *data;
    vnode_ops_t ops;
} vnode_t;

#define VOP_LOOKUP(vn, out, name) vn->ops.lookup(vn, out, name)
#define VOP_CREATE(vn, out, name, vattr) vn->ops.create(vn, out, name, vattr)
#define VOP_LINK(vn, out, name, link, vattr) \
    vn->ops.symlink(vn, out, name, link, vattr)
#define VOP_GETATTR(vn, out) vn->ops.getattr(vn, out)
#define VOP_OPEN(vn, mode) vn->ops.open(vn, mode)
#define VOP_READ(vn, buf, nbytes, off) vn->ops.read(vn, buf, nbytes, off)
#define VOP_WRITE(vn, buf, nbytes, off) vn->ops.write(vn, buf, nbytes, off)
#define VOP_MKDIR(vn, out, name, vattr) vn->ops.mkdir(vn, out, name, vattr)
#define VOP_READDIR(vn, buf, max_size, bytes_read) \
    vn->ops.readdir(vn, buf, max_size, bytes_read)
#define VOP_CLOSE(vn) vn->ops.close(vn)

vnode_t *vfs_open(vnode_t *parent, char *path, int flags);
int vfs_read(vnode_t *vn, void *buf, size_t nbyte, size_t off);
int vfs_write(vnode_t *vn, void *buf, size_t nbyte, size_t off);
int vfs_find_and(vnode_t *cwd, vnode_t **out, const char *path,
                 const char *link, int flags, vattr_t *attr);
int vfs_getdents(vnode_t *vn, void *buf, size_t max_size, size_t *bytes_read);

int vfs_mkdir(vnode_t *vn, vnode_t **out, const char *name, vattr_t *vattr);
int vfs_create(vnode_t *vn, vnode_t **out, const char *name, vattr_t *vattr);

extern vnode_t *root_vnode;

#endif
