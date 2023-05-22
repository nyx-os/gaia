#include <posix/fs/devfs.h>
#include <libkern/base.h>
#include <posix/fs/vfs.h>
#include <posix/fs/cdev.h>

static LIST_HEAD(, devnode) devices;

void devfs_setup_vnode(vnode_t *vn, dev_t dev)
{
    devnode_t *devn;

    bool found = false;

    LIST_FOREACH(devn, &devices, link)
    {
        if (devn->dev == dev) {
            found = true;
            break;
        }
    }

    if (!found) {
        devn = kmalloc(sizeof(devnode_t));
        devn->dev = dev;
        LIST_INSERT_HEAD(&devices, devn, link);
    }

    vn->devnode = devn;
}

int devfs_read(struct vnode *vn, void *buf, size_t nbyte, off_t off)
{
    return cdevs[MAJOR(vn->devnode->dev)].read(vn->devnode->dev, buf, nbyte,
                                               off);
}

int devfs_write(struct vnode *vn, void *buf, size_t nbyte, off_t off)
{
    return cdevs[MAJOR(vn->devnode->dev)].write(vn->devnode->dev, buf, nbyte,
                                                off);
}

int devfs_ioctl(struct vnode *vn, int req, void *out)
{
    return cdevs[MAJOR(vn->devnode->dev)].ioctl(vn->devnode->dev, req, out);
}
