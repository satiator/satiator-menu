/*  Copyright (c) 2017 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdint.h>
#include "../satisfier-types.h"

char file_buf[256];
char *read_file_name(FILE *fp, int offset) {
    fseek(fp, offset, SEEK_SET);
    uint8_t namelen;
    fread(&namelen, 1, 1, fp);
    fread(file_buf, namelen, 1, fp);
    file_buf[namelen] = '\0';
    return file_buf;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: dumpdesc input.desc\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("open");
        return 1;
    }

    uint16_t nseg;
    fread(&nseg, 2, 1, fp);
    printf("%d segments\n", nseg);

    seg_desc_t segs[nseg];
    fread(segs, sizeof(seg_desc_t), nseg, fp);

    for (int i=0; i<nseg; i++) {
        seg_desc_t *seg = &segs[i];
        printf("%02x %02d %02d %8d for %8d",
               seg->q_mode,
               seg->track, seg->index,
               seg->start, seg->length
           );

        if (seg->filename_offset) {
            fseek(fp, seg->filename_offset, SEEK_SET);
            uint8_t namelen;
            fread(&namelen, 1, 1, fp);
            char name[namelen + 1];
            fread(name, namelen, 1, fp);
            name[namelen] = '\0';
            printf(" from %X in %s", seg->file_offset, name);
        }

        printf("\n");
    }

    return 0;
}
