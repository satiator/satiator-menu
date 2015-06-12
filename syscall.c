/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include "satisfier.h"

#undef errno
extern int errno;   // yuck but okay

// Negative numbers indicate errors, nonnegative are results
static int set_errno(int res) {
    if (res >= 0) {
        errno = 0;
        return res;
    }
    switch (-res) {
        case FR_DENIED:
            errno = EACCES;
            break;
        case FR_NO_FILE:
            errno = ENOENT;
            break;
        case FR_NO_PATH:
            errno = ENOTDIR;
            break;
        case FR_INVALID_PARAMETER:
            errno = EINVAL;
            break;
        default:
            errno = EIO;
            break;
    }
    return -1;
}

// obviously don't call this
void __exit(int status) {
    for(;;);
}

int _close(int file) {
    FRESULT ret;
    if (file < 3)   // std handles
        return -1;
    ret = s_close(file - 3);
    return set_errno(ret);
}

char *__env[1] = { 0 };
char **environ = __env;

int _execve(char *name, char **argv, char **env) {
    errno = ENOSYS;
    return -1;
}

int fork() {
    errno = ENOSYS;
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void) {
    return 1;
}

int _isatty(int file) {
    return 0;
}

int _link(char *old, char *new) {
    errno = ENOSYS;
    return -1;
}

int _rename(const char *old, const char *new) {
    FRESULT ret = s_rename((char*)old, (char*)new);
    return set_errno(ret);
}

int _lseek(int file, int ptr, int dir) {
    if (file < 3)
        return 0;
    int whence;
    switch (dir) {
        case SEEK_SET:
            whence = C_SEEK_SET;
            break;
        case SEEK_CUR:
            whence = C_SEEK_CUR;
            break;
        case SEEK_END:
            whence = C_SEEK_END;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    FRESULT ret;
    ret = s_seek(file-3, ptr, whence);
    return set_errno(ret);
}

int _open(const char *name, int flags, ...) {
    int s_flags = 0;
    if (flags & O_RDONLY)
        s_flags |= FA_READ;
    if (flags & O_WRONLY)
        s_flags |= FA_WRITE;
    if (flags & O_TRUNC)
        s_flags |= FA_CREATE_ALWAYS;
    else if (flags & O_CREAT)
        flags |= FA_OPEN_ALWAYS;

    int ret = s_open((char*)name, s_flags);
    if (set_errno(ret) < 0)
        return -1;
    return ret + 3;
}

int _read(int file, char *ptr, int len) {
    if (file < 3)
        return -1;

    int nread = 0;
    while (len) {
        int toread = len;
        if (toread > S_MAXBUF)
            toread = S_MAXBUF;
        int actual = s_read(file-3, ptr, toread);
        if (actual < 0)
            return set_errno(actual);

        nread += actual;
        if (actual < toread)    // EOF, don't bother retrying
            return nread;

        len -= actual;
        ptr += actual;
    }
    return nread;
}
int _write(int file, char *ptr, int len) {
    if (file < 3)
        return -1;

    int nwritten = 0;
    while (len) {
        int towrite = len;
        if (towrite > S_MAXBUF)
            towrite = S_MAXBUF;
        int actual = s_write(file-3, ptr, towrite);
        if (actual < 0)
            return set_errno(actual);

        nwritten += actual;
        if (actual < towrite)    // EOF, don't bother retrying
            return nwritten;

        len -= actual;
        ptr += actual;
    }
    return nwritten;
}

extern char _free_ram_begin, _free_ram_end;
static char *brk = &_free_ram_begin;
caddr_t _sbrk(int incr) {
    char *oldbrk = brk;
    if (brk + incr >= &_free_ram_end) {
        errno = ENOMEM;
        return (void*)-1;
    }

    brk += incr;
    return oldbrk;
}

int _stat(const char *file, struct stat *st) {
    return -1;  // just use s_stat
}
clock_t times(struct tms *buf) {
    return -1;
}
int unlink(char *name) {
    return set_errno(s_unlink(name));
}
int wait(int *status) {
    return -1;
}
