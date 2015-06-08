/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

OUTPUT_FORMAT("elf32-sh", "elf32-sh",
          "elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(_start)

MEMORY {
    /* length (IP load size) determined by header in ip.bin */
    initial (rwx) : org = 0x200000, l = 0x2000
}

SECTIONS {
    .initial : {
        *(.start);
        *(.text);
        *(.rodata);
        *(.data);
        *(.bss);
    } >initial
}
