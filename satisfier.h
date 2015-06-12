/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

// XXX include this from its proper place {{{
typedef enum {
    c_get_status = 0x90,
    c_write_buffer,
    c_read_buffer,

    c_mkfs = 0x94,
    c_fsinfo,
    c_settime,

    c_open = 0xA0,
    c_close,
    c_seek,
    c_read,
    c_write,
    c_truncate,
    c_stat,
    c_rename,
    c_unlink,
    c_mkdir,
    c_opendir,
    c_readdir,
    c_chdir,
    c_emulate,
} satisfier_cmd_t;

#define C_SEEK_SET  0
#define C_SEEK_CUR  1
#define C_SEEK_END  2
// }}}


// XXX fatfs bits {{{
/* File access control and file status flags (FIL.flag) */

#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	0x00

#define	FA_WRITE			0x02
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10

typedef enum {
	FR_OK = 0,				/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,				/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,				/* (4) Could not find the file */
	FR_NO_PATH,				/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,				/* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_SHARE */
	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} FRESULT;
// }}}

// API
typedef struct {
    uint32_t size;
    uint16_t date, time;
    uint8_t attrib;
    char name[];
} __attribute__((packed)) s_stat_t;

int s_open(char *filename, int flags);
int s_close(int fd);
int s_seek(int fd, int offset, int whence);
int s_read(int fd, void *buf, int len);
int s_write(int fd, void *buf, int len);
int s_truncate(int fd);
int s_stat(char *filename, s_stat_t *stat, int statsize);
int s_rename(char *old, char *new);
int s_mkdir(char *filename);
int s_unlink(char *filename);
int s_opendir(char *filename);
int s_chdir(char *filename);
int s_emulate(char *filename);

#define S_MAXBUF    2048
