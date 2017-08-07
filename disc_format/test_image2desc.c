/*  Copyright (c) 2017 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>

#include "cdparse.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: %s input.cue output.desc\n", argv[0]);
        return 1;
    }

    int ret = image2desc(argv[1], argv[2]);

    if (ret) {
        printf("Error: %s\n", cdparse_error_string ? cdparse_error_string : "Unknown");
        return ret;
    }

    return 0;
}
