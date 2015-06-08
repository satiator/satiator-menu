/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

// Crudest possible string primitives.

#include <string.h>

void *memset(void *d, int c, size_t n) {
    while (n--) *(*((unsigned char**)&d))++ = c;
    return NULL; // I mean come on
}

void *memcpy(void *d, const void *s, size_t n) {
    while (n--) *(*((unsigned char**)&d))++ = *(*((unsigned char**)&s))++;
    return NULL;
}

size_t strlen(const char *str) {
    int n = 0;
    while (*str++)
        n++;
    return n;
}
