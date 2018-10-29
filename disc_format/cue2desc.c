/*  Copyright (c) 2017 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <alloca.h>
#include <endian.h>
#include "cdparse.h"


#ifdef TEST
    #define dbgprintf printf
    #include "../satiator-types.h"
#else
    #include "satiator.h"
    #ifdef assert
        #undef assert
    #endif
    #define assert(arg)
#endif

#define error(errstr) \
    do { \
        cdparse_error_string = errstr; \
    } while(0);

static int whitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return 1;
        default:
            return 0;
    }
}

typedef enum {
    e_ok = 0,
    e_no_cue_file,
    e_bad_cue_file,
    e_bad_track_file,
    e_bad_out_file,
} cue2desc_error_t;

typedef struct {
    uint8_t track, index, q_mode;
    uint32_t start, length;
    uint32_t file_offset;
    int filename_index;
    uint16_t secsize;
} segment_t;

int cur_fileindex;
uint32_t cur_filesize;
uint8_t cur_track, cur_q_mode;
uint16_t cur_secsize;
segment_t *segments;
char **filenames;
int n_filenames;
int nseg, open_seg;

static segment_t *alloc_seg(void) {
    nseg++;
    segments = realloc(segments, nseg * sizeof(segment_t));
    segment_t *seg = &segments[nseg-1];
    memset(seg, 0, sizeof(segment_t));
    seg->filename_index = -1;
    return seg;
}

static cue2desc_error_t write_desc_file(const char *filename, segment_t *segments, int nseg) {
    FILE *out = fopen(filename, "wb");
    if (!out) {
        error("Can't open output file");
        return e_bad_out_file;
    }

    uint32_t filename_offsets[n_filenames];

    fseek(out, 2 + nseg * sizeof(seg_desc_t), SEEK_SET);
    for (int i=0; i<n_filenames; i++) {
        filename_offsets[i] = ftell(out);

        uint8_t namelen = strlen(filenames[i]);
        fwrite(&namelen, 1, 1, out);
        fwrite(filenames[i], namelen, 1, out);
    }

    // careful here: Saturn is big-endian but we have to write everything little-endian

    fseek(out, 0, SEEK_SET);
    uint16_t h_nseg = htole16(nseg);
    fwrite(&h_nseg, 2, 1, out);

    for (int i=0; i<nseg; i++) {
        segment_t seg = segments[i];
        seg_desc_t desc;
        memset(&desc, 0, sizeof(desc));

        desc.track = seg.track;
        desc.index = seg.index;
        desc.q_mode = seg.q_mode;
        desc.start = htole32(seg.start);
        desc.length = htole32(seg.length);

        if (seg.filename_index >= 0) {
            desc.secsize = htole16(seg.secsize);
            desc.filename_offset = htole32(filename_offsets[seg.filename_index]);
            desc.file_offset = htole32(seg.file_offset);
        }

        fwrite(&desc, sizeof(desc), 1, out);
    }

    fclose(out);
    return e_ok;
}


static void handle_end_of_track_file(void) {
    if (cur_fileindex < 0)  // no file to end
        return;
    cur_fileindex = -1;

    assert(open_seg >= 0);

    segment_t *open = &segments[open_seg];
    assert(open->length == 0);

    uint32_t remain_bytes = cur_filesize - open->file_offset;
    open->length = remain_bytes / open->secsize;

    open_seg = -1;
}

static cue2desc_error_t handle_file(char *params) {
    handle_end_of_track_file();

    char *filename_start, *mode;

    char *lquot = strchr(params, '"');
    if (lquot) {
        char *rquot = strrchr(params, '"');
        *rquot = '\0';
        filename_start = lquot + 1;
        mode = rquot + 1;
    } else {
        // no quotes, just strip space
        while (*params && whitespace(*params))
            params++;
        char *space = strrchr(params, ' ');
        *space = '\0';
        filename_start = params;
        mode = space + 1;
    }

    // strip any path components from filename
    char *slash;
    if ((slash = strrchr(filename_start, '\\')))
        filename_start = slash+1;

    if ((slash = strrchr(filename_start, '/')))
        filename_start = slash+1;

    char *filename = strdup(filename_start);

    while (*mode && whitespace(*mode))
        mode++;

    n_filenames++;
    filenames = realloc(filenames, sizeof(char*) * n_filenames);
    cur_fileindex = n_filenames - 1;
    filenames[cur_fileindex] = filename;

    struct stat st;
    int ret = stat(filename, &st);
    if (ret < 0) {
        error("Could not stat track file");
        return e_bad_track_file;
    }

    cur_filesize = st.st_size;

    if (strcmp(mode, "BINARY")) {
        error("Bad file mode (not BINARY)");
        return e_bad_cue_file;
    }

    return e_ok;
}

static cue2desc_error_t handle_track(char *params) {
    char *saveptr = NULL;
    char *s_tno = strtok_r(params, " ", &saveptr);
    if (!s_tno) {
        error("Did not find track number");
        return e_bad_cue_file;
    }
    cur_track = strtoul(s_tno, NULL, 10);
    if (!cur_track || cur_track > 99) {
        error("Found invalid track number");
        return e_bad_cue_file;
    }

    char *s_mode = strtok_r(NULL, " ", &saveptr);
    if (!s_mode) {
        error("Did not find track mode");
        return e_bad_cue_file;
    }

    if (!strcmp(s_mode, "MODE1/2048")) {
        cur_q_mode = 0x41;
        cur_secsize = 2048;
    } else if (!strcmp(s_mode, "MODE1/2352")) {
        cur_q_mode = 0x41;
        cur_secsize = 2352;
    } else if (!strcmp(s_mode, "AUDIO")) {
        cur_q_mode = 0x01;
        cur_secsize = 2352;
    } else {
        error("Unknown track mode");
        return e_bad_cue_file;
    }

    return e_ok;
}

static cue2desc_error_t handle_index(char *params) {
    int index, mm, ss, ff;
    int n = sscanf(params, "%d %d:%d:%d", &index, &mm, &ss, &ff);
    if (n != 4) {
        error("Could not parse index");
        return e_bad_cue_file;
    }

    segment_t *seg = alloc_seg();
    seg->track = cur_track;
    seg->index = index;
    seg->q_mode = cur_q_mode;
    seg->secsize = cur_secsize;

    uint32_t fad = ff + 75*(ss + 60*mm);
    seg->filename_index = cur_fileindex;
    seg->file_offset = fad * cur_secsize;

    if (open_seg >= 0) {
        segment_t *open = &segments[open_seg];
        uint32_t remain_bytes = seg->file_offset - open->file_offset;
        open->length = remain_bytes / open->secsize;
    }

    open_seg = nseg-1;

    return e_ok;
}

static cue2desc_error_t handle_pregap(char *params) {
    int mm, ss, ff;
    int n = sscanf(params, "%d:%d:%d", &mm, &ss, &ff);
    if (n != 3) {
        error("Could not parse pregap");
        return e_bad_cue_file;
    }
    uint32_t fad = ff + 75*(ss + 60*mm);

    segment_t *seg = alloc_seg();
    seg->track = cur_track;
    seg->index = 0;
    seg->length = fad;
    seg->q_mode = cur_q_mode;

    return e_ok;
}

int cue2desc(const char *cue_file, const char *desc_file) {
    cue2desc_error_t ret = e_ok;

    FILE *fp = fopen(cue_file, "r");
    if (!fp) {
        error("Could not open specified cue file");
        return e_no_cue_file;
    }

    cur_filesize = cur_track = cur_secsize = 0;
    cur_fileindex = open_seg = -1;
    nseg = 0;
    segments = calloc(1, sizeof(segment_t));
    n_filenames = 0;
    filenames = calloc(1, sizeof(char*));

    char buf[512];
    char *line;

    while ((line = fgets(buf, sizeof(buf), fp))) {
        // Trim whitespace at front
        while (*line && whitespace(*line))
            line++;
        if (!*line)
            continue;

        // Trim whitespace at end
        char *eol = line + strlen(line) - 1;
        while (eol > line && whitespace(*eol))
            *eol-- = '\0';

        // Find command word
        char *space = strchr(line, ' ');
        if (!space) {
            error("No space in line");
            ret = e_bad_cue_file;
            goto out;
        }
        *space = '\0';
        char *params = space + 1;

        if (!strcmp(line, "CATALOG")
         || !strcmp(line, "CDTEXTFILE")
         || !strcmp(line, "FLAGS")
         || !strcmp(line, "ISRC")
         || !strcmp(line, "PERFORMER")
         || !strcmp(line, "POSTGAP")
         || !strcmp(line, "REM")
         || !strcmp(line, "SONGWRITER")
         || !strcmp(line, "TITLE"))
            continue;   // recognised, but unsupported

        if (!strcmp(line, "FILE"))
            ret = handle_file(params);
        else if (!strcmp(line, "TRACK"))
            ret = handle_track(params);
        else if (!strcmp(line, "INDEX"))
            ret = handle_index(params);
        else if (!strcmp(line, "PREGAP"))
            ret = handle_pregap(params);
        else
            ret = e_bad_cue_file;

        if (ret != e_ok)
            goto out;
    }

    handle_end_of_track_file();

    uint32_t disc_fad = 150;
    for (int i=0; i<nseg; i++) {
        segments[i].start = disc_fad;
        disc_fad += segments[i].length;
    }

    ret = write_desc_file(desc_file, segments, nseg);

out:
    for (int i=0; i<n_filenames; i++)
        free(filenames[i]);
    free(filenames);

    free(segments);
    fclose(fp);
    return ret;
}
