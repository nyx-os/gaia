#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H

typedef long off_t;
typedef long off64_t;
typedef long ino_t;
typedef unsigned int mode_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef int tid_t;
typedef long time_t;
typedef long blkcnt_t;
typedef long blksize_t;
typedef unsigned long dev_t;
typedef unsigned long nlink_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#endif
