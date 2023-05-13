#include <posix/fs/vfs.h>
#include <posix/fs/cdev.h>
#include <kern/term/term.h>

static int tty_read(dev_t dev, void *buf, size_t nbyte, size_t off)
{
    (void)dev;
    (void)buf;
    (void)nbyte;
    (void)off;
    panic("tty_read is a stub");
    return 0;
}

static int tty_write(dev_t dev, void *buf, size_t nbyte, size_t off)
{
    (void)dev;
    (void)buf;
    (void)nbyte;
    (void)off;
    term_write((char *)buf);
    return nbyte;
}

void tty_init(void)
{
    vnode_t *vn;

    cdev_t cdev = {
        .is_atty = true,
        .read = tty_read,
        .write = tty_write,
    };

    root_devnode->ops.mknod(root_devnode, &vn, "tty",
                            MKDEV(cdev_allocate(&cdev), 0));
}
