/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>
#include "satisfier.h"

// Reset everything we can before loading the menu.
void sysinit(void) {
   int i;
   interrupt_set_level_mask(0xF);
   for (i = 0; i < 0x80; i++)
      bios_set_sh2_interrupt(i, 0);
   for (i = 0x40; i < 0x60; i++)
      bios_set_scu_interrupt(i, 0);

   // Make sure all interrupts have been called
   bios_change_scu_interrupt_mask(0, 0);
   bios_change_scu_interrupt_mask(0xFFFFFFFF, 0xFFFFFFFF);

   vdp_init(RES_320x224);
   per_init();

   // If DSP is running, stop it
   if (dsp_is_exec())
       dsp_stop();

   if (interrupt_get_level_mask() > 0x7)
      interrupt_set_level_mask(0x7);
}

// wipe off the menu neatly before we come up
void fadeout(void);

void start(void) __attribute__((section(".start")));
void start(void) {
    fadeout();
    sysinit();

    char buf[16];
    buf[0] = 'f';
    buf[1] = 0;

    int handle = s_open(buf, FA_READ|FA_WRITE|FA_CREATE_ALWAYS);

    for(;;);
}
