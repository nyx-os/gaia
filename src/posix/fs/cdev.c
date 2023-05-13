#include <posix/fs/cdev.h>

cdev_t cdevs[64];

int cdev_allocate(cdev_t *dev)
{
    for (int i = 0; i < 64; i++)
        if (!cdevs[i].present) {
            cdevs[i] = *dev;
            cdevs[i].present = true;
            return i;
        }

    /* out of majors */
    return -1;
}
