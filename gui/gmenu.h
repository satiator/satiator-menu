/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iapetus.h>
#include "filelist.h"

void menu_init(void);
void menu_error(const char *title, const char *message);
int menu_picklist(const file_ent *entries, int n_entries, const char *caption, font_struct *font);
