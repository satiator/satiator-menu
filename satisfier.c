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

static inline void sat_cmd(cmd_t cr, uint16_t wait) {
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
static inline void sat_stat(void) {
    cmd_t cmd = {c_get_status<<8, 0, 0, 0};
    sat_cmd(cmd, 0);
    sat_result[0] = CDB_REG_CR1;
    sat_result[1] = CDB_REG_CR2;
    sat_result[2] = CDB_REG_CR3;
    sat_result[3] = CDB_REG_CR4;
}

static inline int buffer_xfer(void *buf, int len, int dir) {
    uint8_t cmdbyte = dir ? c_write_buffer : c_read_buffer;
    cmd_t cmd = {cmdbyte<<8, 0, 0, len};
    sat_cmd(cmd, HIRQ_EHST);
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

// File API {{{
int s_open(char *filename, int flags) {
    buffer_write(filename, strlen(filename));
    cmd_t cmd = {c_open<<8, flags, 0, strlen(filename)};
    sat_cmd(cmd, HIRQ_MPED);
    sat_stat();

    uint8_t retval = sat_result[0] >> 8;
    if (retval)
        return -retval;

    return sat_result[3];   // handle
}
// }}}
