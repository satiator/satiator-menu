#include <iapetus.h>
#include <stdio.h>
#include "satisfier.h"
#include <string.h>
#include "cdparse.h"
#include "fade.h"
#include "syscall.h"
#include "gmenu.h"
#include "jhloader.h"

static file_ent * list_files(const char *dir, int *entries) {
    *entries = 0;
    if (s_opendir(dir))
        return NULL;
    char statbuf[280];
    s_stat_t *st = (s_stat_t*)statbuf;
    int nfiles = 0;
    int listsz = 8;
    file_ent *list = malloc(listsz * sizeof(file_ent));
    if (!list)
        return NULL;
    int len;
    while ((len = s_stat(NULL, st, sizeof(statbuf)-1)) > 0) {
        st->name[len] = 0;
        if (!strcmp(st->name, "."))
            continue;
        if (st->attrib & AM_DIR) {
            list[nfiles].name = malloc(len + 2);
            strcpy(list[nfiles].name, st->name);
            list[nfiles].name[len] = '/';
            list[nfiles].name[len+1] = 0;
            list[nfiles].isdir = 1;
        } else {
            if (len < 4)
                continue;
            if (strcasecmp(&st->name[len-4], ".cue") &&
                strcasecmp(&st->name[len-4], ".iso"))
                continue;

            list[nfiles].name = strdup(st->name);
            list[nfiles].isdir = 0;
        }
        if (!list[nfiles].name) {
            free(list);
            return NULL;
        }

        nfiles++;
        if (nfiles == listsz) {
            listsz *= 2;
            list = realloc(list, listsz * sizeof(file_ent));
        }
    }
    *entries = nfiles;
    return list;
}

static int compar_list(const void *pa, const void *pb) {
    const file_ent *a = pa, *b = pb;
    if (a->isdir && !b->isdir)
        return -1;
    if (!a->isdir && b->isdir)
        return 1;
    return strcmp(a->name, b->name);
}

static void sort_list(file_ent *list, int n_entries) {
    qsort(list, n_entries, sizeof(file_ent), compar_list);
}

static void free_list(file_ent *list, int n_entries) {
    int i;
    for (i=0; i<n_entries; i++)
        free(list[i].name);
    free(list);
}

void launch_game(const char *filename) {
    dbgprintf("Loading ISO: '%s'\n", filename);
    int ret = image2desc(filename, "out.desc");
    if (ret) {
        menu_error("Disc load failed!", cdparse_error_string ? cdparse_error_string : "Unknown error");
        return;
    }

    fadeout(0x20);
    dbgprintf("loading stubloader.desc\n");
    /* s_emulate("stubloader.desc"); */
    s_emulate("out.desc");
    dbgprintf("lid wait open...\n");
    while (is_cd_present());
    dbgprintf("lid wait close...\n");
    while (!is_cd_present());
    s_mode(S_MODE_CDROM);
    dbgprintf("boop\n");
    ret = boot_disc();

    s_mode(S_MODE_USBFS);   // failed, restore order
    s_emulate("");  // close the old file
    fadein(0x20);

    const char *error = "Unknown error";
    switch(ret) {
        case BOOT_BAD_HEADER:
            error = "Bad disc header. Bad image?";
            break;
        case BOOT_BAD_REGION:
            error = "Wrong region.";
            break;
        case BOOT_BAD_SECURITY_CODE:
            error = "Bad security code. Bad image?";
            break;
    }
    menu_error("Boot failed!", error);
}


void main_menu(void) {
    menu_init();

#ifdef DEBUG
    if (ud_detect() == IAPETUS_ERR_OK)
        syscall_enable_stdout_ud = 1;
#endif

    char *name = NULL;
    for(;;) {
        int nents;
        file_ent *list = list_files(".", &nents);
        sort_list(list, nents);
        char namebuf[256], pathbuf[256];
        strcpy(namebuf, "Satiator - ");
        s_getcwd(pathbuf, sizeof(pathbuf));
        strcat(namebuf, pathbuf);
        int entry = menu_picklist(list, nents, namebuf, NULL);
        if (entry == -1)
            s_chdir("..");
        else if (list[entry].isdir)
            s_chdir(list[entry].name);
        else
            name = strdup(list[entry].name);
        free_list(list, nents);

        if (name) {
            launch_game(name);
            free(name);
            name = NULL;
        }
    }
}
