#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H
#include <sys/types.h>

#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)

#define MINOR(dev) ((unsigned int)((dev) >> MINORBITS))
#define MAJOR(dev) ((unsigned int)((dev)&MINORMASK))
#define MKDEV(ma, mi) (((ma) << MINORBITS) | (mi))

typedef long time_t;
typedef long blksize_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#endif
