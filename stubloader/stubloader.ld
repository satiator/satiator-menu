/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

OUTPUT_FORMAT("elf32-sh", "elf32-sh",
          "elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(saveregs)

MEMORY {
    disc_boot (rwx) : org = 0x06002f00, l = 0x7000
    work_low (rwx) : org = 0x200000, l = 0x100000
    /* need somewhere to deposit our back-to-menu hook */
    bios_hook (rwx) : org = 0x600042c, l = 0x100
}

SECTIONS {
    .startup : {
        *(.text.saveregs);
        *(.start);
        *(.start.*);
        __ram_origin = .;
    } >disc_boot

    .ram : AT(__ram_origin) {
        __ram_start = .;
        *(.text);
        *(.text.*);
        *(.rodata);
        *(.rodata.*);
        *(.data);
        *(.data.*);
        __ram_end = .;
    } >work_low

    .startup2 : {
        . = . + SIZEOF(.ram);
        __bios_hook_origin = .;
    }

    .bios_hook : AT(__bios_hook_origin) {
        __bios_hook_start = .;
        *(.bios_hook);
        *(.bios_hook.*);
        *(.bios_hook_data);
        *(.bios_hook_data.*);
        __bios_hook_end = .;
    } >bios_hook

    .ram_uninit : AT(ADDR(.ram) + SIZEOF(.ram)) {
        *(.bss);
        *(.bss.*);
        *(COMMON);
        __bss_end = .;
        . = ALIGN(4);
        __free_ram_begin = __ram_end;
        __free_ram_end = __ram_start + 0x100000;
    } >work_low

}
