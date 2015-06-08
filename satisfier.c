/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>
#include "satisfier.h"
#include <string.h>

// I/O primitives   {{{

typedef uint16_t cmd_t[4];

static inline void exec_cmd(cmd_t cr, uint16_t wait) {
    CDB_REG_HIRQ = 0;
    CDB_REG_CR1 = cr[0];
    CDB_REG_CR2 = cr[1];
    CDB_REG_CR3 = cr[2];
    CDB_REG_CR4 = cr[3];
    while (!(CDB_REG_HIRQ & HIRQ_CMOK));
    if (wait)
        while (!(CDB_REG_HIRQ & wait));
}

static uint16_t sat_result[4];
static inline void get_stat(void) {
    cmd_t cmd = {c_get_status<<8, 0, 0, 0};
    exec_cmd(cmd, 0);
    sat_result[0] = CDB_REG_CR1;
    sat_result[1] = CDB_REG_CR2;
    sat_result[2] = CDB_REG_CR3;
    sat_result[3] = CDB_REG_CR4;
}

static inline int buffer_xfer(void *buf, int len, int dir) {
    uint8_t cmdbyte = dir ? c_write_buffer : c_read_buffer;
    cmd_t cmd = {cmdbyte<<8, 0, 0, len};
    exec_cmd(cmd, HIRQ_EHST);
    if (CDB_REG_CR1)  // error
        return -1;

    while (!(CDB_REG_HIRQ & HIRQ_DRDY));

    uint16_t *p = buf;
    uint16_t n = (len+1)/2;
    if (dir) {
        while (n--)
            CDB_REG_DATATRNS16 = *p++;
    } else {
        while (n--)
            *p++ = CDB_REG_DATATRNS16;
    }

    return 0;
}

#define buffer_read(buf, len)  buffer_xfer(buf, len, 0)
#define buffer_write(buf, len) buffer_xfer(buf, len, 1)

// }}}

// Convenience functions {{{
// Most calls use a similar pattern
static inline void set_cmd(cmd_t cmd, int op, int fd, int flags, int len) {
    cmd[0] = (op<<8) | (fd & 0xf);
    cmd[1] = flags;
    cmd[2] = len>>16;
    cmd[3] = len;
}

// Most API returns a negative number for errors
#define get_check_stat() do {               \
    get_stat();                             \
    uint8_t __retval = sat_result[0] >> 8;  \
    if (__retval)                           \
        return __retval;                    \
} while(0);

#define get_length()    ((sat_result[2]<<16) | sat_result[3])

// Most calls use the same sequence
#define simplecall(...) do {                \
    cmd_t cmd;                              \
    set_cmd(cmd, __VA_ARGS__);              \
    exec_cmd(cmd, HIRQ_MPED);               \
    get_check_stat();                       \
} while(0);

// }}}

// File API {{{
// Given a filename and some FA_xxx flags, return a file descriptor
// (or a negative error corresponding to FR_xxx)
int s_open(char *filename, int flags) {
    buffer_write(filename, strlen(filename));
    simplecall(c_open, 0, flags, strlen(filename));
    return sat_result[3];   // handle
}

// Close a file descriptor
int s_close(int fd) {
    simplecall(c_close, fd, 0, 0);
    return 0;
}

// Seek to a byte on an fd. Returns the offset
int s_seek(int fd, int offset) {
    simplecall(c_seek, fd, 0, offset);
    return (sat_result[2]<<16) | sat_result[3];
}

// Read some data. Returns bytes read
int s_read(int fd, void *buf, int len) {
    if (len > 2048 || len < 0)
        return FR_INVALID_PARAMETER;
    simplecall(c_read, fd, 0, len);
    len = get_length();
    buffer_read(buf, len);
    return len;
}

// Write some data. Returns bytes written
int s_write(int fd, void *buf, int len) {
    if (len > 2048 || len < 0)
        return FR_INVALID_PARAMETER;
    buffer_write(buf, len);
    simplecall(c_write, fd, 0, len);
    return get_length();
}

// Truncate file at current pointer. Returns new length
int s_truncate(int fd) {
    simplecall(c_truncate, fd, 0, 0);
    return get_length();
}

// Get info on a named file. Pass in a pointer to a buffer and
// its size - the filename may be truncated if the buffer is
// short.  Returns the length of the (truncated) filename.
// If the filename is NULL, reads the next file from the
// current directory (readdir).
int s_stat(char *filename, s_stat_t *stat, int statsize) {
    int len;
    if (statsize < 9)
        return FR_INVALID_PARAMETER;
    if (filename) {
        len = strlen(filename);
        buffer_write(filename, len);
    } else {
        len = 0;
    }
    simplecall(filename ? c_stat : c_readdir, 0, 0, len);
    len = get_length();
    if (len > statsize)
        len = statsize;
    buffer_read(stat, len);
    return len - 9;
}

// Rename a file.
int s_rename(char *old, char *new) {
    // Need both names, zero separated, for the write
    char namebuf[512];  // nasty huh
    int len1 = strlen(old);
    int len2 = strlen(new);
    memcpy(namebuf, old, len1+1);
    memcpy(namebuf+len1+1, new, len2);
    buffer_write(namebuf, len1+1+len2);
    simplecall(c_rename, 0, 0, len1+1+len2);
    return 0;
}

// Create a directory.
int s_mkdir(char *filename) {
    int len = strlen(filename);
    buffer_write(filename, len);
    simplecall(c_mkdir, 0, 0, len);
    return 0;
}

// Delete a file.
int s_unlink(char *filename) {
    int len = strlen(filename);
    buffer_write(filename, len);
    simplecall(c_unlink, 0, 0, len);
    return 0;
}

// Open a directory to read file entries.
int s_opendir(char *filename) {
    int len = strlen(filename);
    buffer_write(filename, len);
    simplecall(c_opendir, 0, 0, len);
    return 0;
}

// Change working directory.
int s_chdir(char *filename) {
    int len = strlen(filename);
    buffer_write(filename, len);
    simplecall(c_chdir, 0, 0, len);
    return 0;
}
// }}}
