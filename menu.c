/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>
#include <string.h>
#include <stdio.h>
#include "satisfier.h"
#include "menu.h"

static font_struct main_font;

void menu_init(void) {
    screen_settings_struct settings;
    // Setup a screen for us draw on
    settings.is_bitmap = TRUE;
    settings.bitmap_size = BG_BITMAP512x256;
    settings.transparent_bit = 0;
    settings.color = BG_256COLOR;
    settings.special_priority = 0;
    settings.special_color_calc = 0;
    settings.extra_palette_num = 0;
    settings.map_offset = 0;
    settings.rotation_mode = 0;
    settings.parameter_addr = 0x25E60000;
    vdp_rbg0_init(&settings);

    // Use the default palette
    vdp_set_default_palette();

    // Setup the default 8x16 1BPP font
    main_font.data = font_8x8;
    main_font.width = 8;
    main_font.height = 8;
    main_font.bpp = 1;
    main_font.out = (u8 *)0x25E00000;
    vdp_set_font(SCREEN_RBG0, &main_font, 1);

    // Display everything
    vdp_disp_on();
}

#define SCROLL_OFF 3

int menu_picklist(file_ent *entries, int n_entries, char *caption, font_struct *font) {
    int width, height;
    int old_transparent;

    if (!font)
        font = &main_font;

    vdp_get_scr_width_height(&width, &height);

    gui_window_init();

    old_transparent = font->transparent;
    font->transparent = 1;

    int selected = 0;
    int scrollbase = 0;
    int draw_base_y = 32;   // ?
    int n_rows = (height - draw_base_y - 32) / font->height;
    int n_cols = (width - 64) / font->width;

    gui_window_draw(8, 8, width-16, height-16, TRUE, 0, RGB16(26, 26, 25) | 0x8000);
    for(;;) {
        vdp_clear_screen(font);
        char namebuf[256];
        s_getcwd(namebuf, sizeof(namebuf));
        vdp_print_text(font, 8+6, 8+4, 0xf, caption);
        // draw some entries
        int i;
        for (i=0; i<n_rows; i++) {
            int entry = i + scrollbase;
            if (entry >= n_entries)
                break;
            if (entry == selected)
                vdp_print_text(font, 16, draw_base_y + font->height*i, 0x10, ">");

            // truncate name so as not to overrun screen
            char namebuf[n_cols];
            strncpy(namebuf, entries[entry].name, sizeof(namebuf));
            vdp_print_text(font, 32, draw_base_y + font->height*i, 0x10, namebuf);
        }
        if (scrollbase > 0)
            vdp_print_text(font, width*3/4, draw_base_y-4, 0x10, "^");
        if (scrollbase + n_rows < n_entries)
            vdp_print_text(font, width*3/4, draw_base_y+font->height*i-4, 0x10, "v");

        // wait for input
        for(;;) {
            vdp_vsync();
            if (per[0].but_push_once & PAD_UP) {
                selected--;
                goto move;
            }
            if (per[0].but_push_once & PAD_DOWN) {
                selected++;
                goto move;
            }
            if (per[0].but_push_once & PAD_A) {
                goto out;
            }
            if (per[0].but_push_once & PAD_B) {
                selected = -1;
                goto out;
            }
        };
move:
        if (selected >= n_entries)
            selected = n_entries - 1;
        if (selected < 0)
            selected = 0;
        if (selected - scrollbase < SCROLL_OFF && scrollbase > 0)
            scrollbase--;
        if (scrollbase + n_rows - selected < SCROLL_OFF && scrollbase < n_entries - n_rows)
            scrollbase++;
    }

out:
    font->transparent = old_transparent;
    return selected;
}
