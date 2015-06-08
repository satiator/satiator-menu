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
    initial (rwx) : org = 0x200000, l = 0x8000
}

SECTIONS {
    .initial : {
        __load_start = .;
        *(.start);
        *ashiftrt.o(.text); /* necessary for the BIOS load */

        *(.text);
        *(.rodata);
        *(.rodata.*);
        *(.data);
        __load_end = .;
        *(.bss);
        __bss_end = .;
    } >initial
}
