/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>

static volatile int fadeval;

static void fadeisr(void) {
    fadeval -= 10;
    if (fadeval < 0x100)
        fadeval = 0x100;
    
    VDP2_REG_COAR = fadeval;
    VDP2_REG_COAG = fadeval;
    VDP2_REG_COAB = fadeval;
}

void fadeout(void) {
    fadeval = 0x1ff;
    bios_change_scu_interrupt_mask(0xffffff, MASK_VBLANKIN);
    bios_set_scu_interrupt(0x40, fadeisr);
    bios_change_scu_interrupt_mask(~MASK_VBLANKIN, 0);
    while (fadeval > 0x100);
    bios_change_scu_interrupt_mask(0xffffff, MASK_VBLANKIN);
}
