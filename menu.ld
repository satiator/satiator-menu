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
    /* all of low Work RAM is free for our use, woo */
    work_low (rwx) : org = 0x200000, l = 0x100000
}

SECTIONS {
    .ram : {
        __load_start = .;
        *(.start);
        *ashiftrt.o(.text); /* necessary for the BIOS load */

        *(.text);
        *(.text.*);
        *(.rodata);
        *(.rodata.*);
        *(.data);
        *(.data.*);
        __load_end = .;
        *(.bss);
        *(.bss.*);
        *(COMMON);
        __bss_end = .;
        . = ALIGN(4);
        __free_ram_begin = .;
        __free_ram_end = __load_start + 0x100000;
    } >work_low
}
