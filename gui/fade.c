/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>

void fadeout(int step) {
    int fadeval = 0x1ff;

    while (fadeval > 0x100) {
        while(!(VDP2_REG_TVSTAT & 8));
        fadeval -= step;
        if (fadeval < 0x100)
            fadeval = 0x100;

        VDP2_REG_COAR = fadeval;
        VDP2_REG_COAG = fadeval;
        VDP2_REG_COAB = fadeval;

        VDP2_REG_CLOFEN = 0xffff;

        while(VDP2_REG_TVSTAT & 8);
    }
}

void fadein(int step) {
    int fadeval = 0x100;

    while (fadeval < 0x1ff) {
        while(!(VDP2_REG_TVSTAT & 8));
        fadeval += step;
        if (fadeval > 0x1ff)
            fadeval = 0x1ff;

        VDP2_REG_COAR = fadeval;
        VDP2_REG_COAG = fadeval;
        VDP2_REG_COAB = fadeval;

        VDP2_REG_CLOFEN = 0xffff;

        while(VDP2_REG_TVSTAT & 8);
    }
}
