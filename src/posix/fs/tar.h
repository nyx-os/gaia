#ifndef POSIX_FS_TAR_H
#define POSIX_FS_TAR_H

#define TAR_NORMAL_FILE '0'
#define TAR_DIRECTORY '5'
#define TAR_SYMLINK '2'

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char dev_major[8];
    char dev_minor[8];
    char prefix[155];
} tar_header_t;

void tar_write_on_tmpfs(void *archive);

#endif
