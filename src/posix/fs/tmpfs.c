#include <posix/fs/tmpfs.h>
#include <posix/fs/vfs.h>
#include <posix/dirent.h>
#include <libkern/base.h>
#include <posix/errno.h>
#include <sys/queue.h>

extern vnode_ops_t tmpfs_ops;

static int tmpfs_make_vnode(vnode_t **out, tmp_node_t *node)
{
    if (node->vnode) {
        *out = node->vnode;
        return 0;
    } else {
        vnode_t *vn = kmalloc(sizeof(vnode_t));
        node->vnode = vn;
        vn->type = node->attr.type;
        vn->ops = tmpfs_ops;
        vn->data = node;
        *out = vn;
        return 0;
    }
}

static int tmpfs_lookup(vnode_t *vn, vnode_t **out, const char *name);

static tmp_node_t *tmpfs_make_node(tmp_node_t *dir, vnode_type_t type,
                                   const char *name, const char *link,
                                   vattr_t *attr)
{
    tmp_node_t *n = kmalloc(sizeof(tmp_node_t));
    tmp_dirent_t *dirent = kmalloc(sizeof(tmp_dirent_t));

    memcpy((void *)dirent->name, name, MAX(strlen(name), 256));

    dirent->node = n;

    n->attr = attr ? *attr : (vattr_t){ 0 };

    n->attr.type = type;
    n->attr.size = 0;
    n->vnode = NULL;

    switch (type) {
    case VREG:
        n->data.reg.buffer = NULL;
        break;

    case VDIR:
        LIST_INIT(&n->data.dir.entries);
        n->data.dir.parent = dir;
        break;

    case VLNK: {
        vnode_t *vnode;
        tmpfs_lookup(dir->vnode, &vnode, link);

        n->data.link.to = vnode ? vnode->data : NULL;
        n->data.link.to_name = kmalloc(strlen(link) + 1);
        memcpy(n->data.link.to_name, link, strlen(link));

        break;
    }

    default:
        break;
    }

    LIST_INSERT_HEAD(&dir->data.dir.entries, dirent, link);

    return n;
}

static int strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == 0)
            return (0);
    return (*(const unsigned char *)s1 - *(const unsigned char *)--s2);
}

static tmp_dirent_t *lookup_dirent(tmp_node_t *vn, const char *name)
{
    tmp_dirent_t *dent;

    LIST_FOREACH(dent, &vn->data.dir.entries, link)
    {
        if (strcmp(dent->name, name) == 0)
            return dent;
    }

    return NULL;
}

static int tmpfs_lookup(vnode_t *vn, vnode_t **out, const char *name)
{
    tmp_node_t *node = (tmp_node_t *)(vn->data);
    tmp_dirent_t *ent;

    if (node->attr.type != VDIR) {
        return -ENOTDIR;
    }

    ent = lookup_dirent(node, name);

    if (!ent) {
        return -ENOENT;
    }

    if (ent->node->vnode->type == VLNK) {
        if (ent->node->data.link.to == NULL) {
            vnode_t *link_to;
            tmpfs_lookup(vn, &link_to, ent->node->data.link.to_name);

            ent->node->data.link.to = link_to->data;
        }
    }

    return tmpfs_make_vnode(out, ent->node);
}

static int tmpfs_symlink(vnode_t *dir, vnode_t **out, const char *path,
                         const char *link, vattr_t *attr)
{
    if (dir->type != VDIR) {
        return -ENOTDIR;
    }

    tmp_node_t *n;

    n = tmpfs_make_node((tmp_node_t *)dir->data, VLNK, path, link, attr);

    return tmpfs_make_vnode(out, n);
}

static int tmpfs_create(vnode_t *vn, vnode_t **out, const char *name,
                        vattr_t *attr)
{
    if (vn->type != VDIR) {
        return -ENOTDIR;
    }

    tmp_node_t *n;

    n = tmpfs_make_node((tmp_node_t *)vn->data, VREG, name, NULL, attr);

    return tmpfs_make_vnode(out, n);
}

static int tmpfs_read(vnode_t *vn, void *buf, size_t nbyte, size_t off)
{
    tmp_node_t *tn = (tmp_node_t *)vn->data;

    if (tn->attr.type == VLNK) {
        tn = tn->data.link.to;
    }

    if (tn->attr.type != VREG) {
        return tn->attr.type == VDIR ? -EISDIR : -EINVAL;
    }

    if (off + nbyte > tn->attr.size) {
        nbyte = (tn->attr.size <= off) ? 0 : tn->attr.size - off;
    }

    if (nbyte == 0)
        return 0;

    memcpy(buf, (uint8_t *)tn->data.reg.buffer + off, nbyte);

    return nbyte;
}

static int tmpfs_write(vnode_t *vn, void *buf, size_t nbyte, size_t off)
{
    tmp_node_t *tn = (tmp_node_t *)vn->data;

    bool resize = false;
    size_t origsize = tn->attr.size;

    if (tn->attr.type == VLNK) {
        tn = tn->data.link.to;
    }

    if (tn->attr.type != VREG) {
        return tn->attr.type == VDIR ? -EISDIR : -EINVAL;
    }

    if (off + nbyte > tn->attr.size) {
        tn->attr.size = off + nbyte;
        resize = true;
    }

    if (!tn->data.reg.buffer) {
        tn->data.reg.buffer = kmalloc(tn->attr.size);
    } else if (resize) {
        tn->data.reg.buffer =
                krealloc(tn->data.reg.buffer, origsize, tn->attr.size);
    }

    if (tn->attr.size > 0)
        memcpy((uint8_t *)tn->data.reg.buffer + off, buf, nbyte);

    return nbyte;
}

static int tmpfs_make_new_dir(vnode_t *vn, vnode_t **out, const char *name,
                              vattr_t *vattr)
{
    tmp_node_t *n;
    if (vn->type != VDIR) {
        return -ENOTDIR;
    }

    n = tmpfs_make_node(vn->data, VDIR, name, NULL, vattr);

    if (!n) {
        panic("tmpfs_make_node returned NULL");
    }

    return tmpfs_make_vnode(out, n);
}

static int tmpfs_mkdir(vnode_t *vn, vnode_t **out, const char *name,
                       vattr_t *vattr)
{
    int r;
    vnode_t *n;
    tmp_node_t *tn;

    r = tmpfs_make_new_dir(vn, out, name, vattr);

    r = tmpfs_make_new_dir(*out, &n, ".", NULL);

    tn = (tmp_node_t *)(*out)->data;

    tn->attr.type = VDIR;
    n->type = VDIR;

    n->data = tn;

    r = tmpfs_make_new_dir(*out, &n, "..", NULL);

    n->data = tn->data.dir.parent;

    return r;
}

static int tmpfs_getattr(vnode_t *vn, vattr_t *attr)
{
    tmp_node_t *tn = (tmp_node_t *)vn->data;

    if (!attr)
        return -EINVAL;

    *attr = tn->attr;
    return 0;
}

static int tmpfs_readdir(vnode_t *vn, void *buf, size_t max_size,
                         size_t *bytes_read)
{
    tmp_node_t *tn = (tmp_node_t *)vn->data;

    if (tn->attr.type != VDIR) {
        return -ENOTDIR;
    }

    struct dirent *dent = buf;
    size_t bytes_written = 0;

    tmp_dirent_t *tdent;

    LIST_FOREACH(tdent, &tn->data.dir.entries, link)
    {
        if (bytes_written + sizeof(struct dirent) > max_size) {
            break;
        }

        if (tdent->node->attr.type == VDIR)
            dent->d_type = DT_DIR;
        else if (tdent->node->attr.type == VREG)
            dent->d_type = DT_REG;
        else
            dent->d_type = DT_UNKNOWN;
        dent->d_ino = (uintptr_t)tdent->node;
        dent->d_off++;
        dent->d_reclen = sizeof(struct dirent);

        strncpy(dent->d_name, tdent->name, strlen(tdent->name));

        dent = (struct dirent *)((uint8_t *)dent + sizeof(struct dirent));

        bytes_written += sizeof(struct dirent);
    }

    if (bytes_read)
        *bytes_read = bytes_written;

    return 0;
}

vnode_ops_t tmpfs_ops = { .create = tmpfs_create,
                          .mkdir = tmpfs_mkdir,
                          .read = tmpfs_read,
                          .write = tmpfs_write,
                          .getattr = tmpfs_getattr,
                          .readdir = tmpfs_readdir,
                          .symlink = tmpfs_symlink,
                          .lookup = tmpfs_lookup };

void tmpfs_init(void)
{
    tmp_node_t *root_node = kmalloc(sizeof(tmp_node_t));
    root_node->attr.type = VDIR;
    root_node->vnode = NULL;

    vnode_t *n;

    tmpfs_make_vnode(&root_vnode, root_node);

    root_node->vnode = root_vnode;
    root_vnode->type = VDIR;

    tmpfs_make_new_dir(root_vnode, &n, "..", NULL);

    n->data = root_node;

    tmpfs_make_new_dir(root_vnode, &n, ".", NULL);

    n->data = root_node;
}
