#include <iapetus.h>
#include <stdio.h>
#include "satisfier.h"
#include <string.h>
#include "disc_format/cdparse.h"

void test_file_io(void) {
    // hacky file testing.
    char buf[512];
    int i;
    buf[0] = 'f';
    buf[1] = 0;

    int fd = s_open("foo.bar", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    for (i=0; i<128; i++)
        buf[i] = 3*i;
    s_write(fd, buf, 128);
    s_close(fd);

    fd = s_open("foo.bar", FA_READ|FA_WRITE);
    s_seek(fd, 96, C_SEEK_SET);
    s_truncate(fd);
    s_seek(fd, 48, C_SEEK_SET);
    s_read(fd, buf, 32);
    for (i=0; i<32; i++)
        buf[i] &= 0xf0;
    s_seek(fd, 48, C_SEEK_SET);
    s_write(fd, buf, 32);
    s_close(fd);

    s_rename("foo.bar", "baz.foo");
    
    i = s_stat("baz.foo", (void*)buf, sizeof(buf));
    s_stat_t * stat = (void*)buf;
    fd = s_open("out", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    s_write(fd, stat, i+9);
    s_close(fd);

    s_mkdir("blonk");

    s_opendir(".");
    fd = s_open("out2", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    while ((i = s_stat(NULL, stat, sizeof(buf))) > 0) {
        s_write(fd, stat, i+9);
    }
    s_close(fd);

    s_chdir("blonk");
    s_rename("../out2", "bloop");

    s_chdir("..");
    s_unlink("baz.foo");
}

void test_stdio(void) {
    FILE *fp = fopen("stdio.txt", "w");
    fprintf(fp, "Hello world!\n");
    fwrite("123456", 6, 1, fp);
    fputc(0x55, fp);
    fclose(fp);
}

#include "menu.h"
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
void fadeout(int arg);
void test_menu(void) {
    menu_init();
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
            dprintf("Loading ISO: '%s'\n", name);
            int ret = load_iso(name);
            fadeout(0x20);
            if (!ret)
                s_emulate("stubloader.desc");
            free(name);
            name = NULL;
        }
    }
}
