#include <iapetus.h>
#include <stdio.h>
#include "satiator.h"
#include <string.h>
#include "cdparse.h"
#include "fade.h"
#include "syscall.h"
#include "gmenu.h"
#include "jhloader.h"

int image_file_filter(file_ent *entry) {
    if (entry->isdir)
        return 1;

    int len = strlen(entry->name);
    if (!strcasecmp(&entry->name[len-4], ".cue"))
        return 1;
    if (!strcasecmp(&entry->name[len-4], ".iso"))
        return 1;

    return 0;
}

void launch_game(const char *filename) {
    dbgprintf("Loading ISO: '%s'\n", filename);
    int ret = image2desc(filename, "out.desc");
    if (ret) {
        menu_error("Disc load failed!", cdparse_error_string ? cdparse_error_string : "Unknown error");
        return;
    }

    fadeout(0x20);

    restore_vdp_mem();

    s_emulate("out.desc");
    while (is_cd_present());
    while (!is_cd_present());
    s_mode(s_cdrom);
    ret = boot_disc();

    s_mode(s_api);   // failed, restore order
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

char test_buf[128];

char pathbuf[512];
void main_menu(void) {
    menu_init();

#ifdef DEBUG
    if (ud_detect() == IAPETUS_ERR_OK)
        syscall_enable_stdout_ud = 1;
#endif

    char *name = NULL;
    for(;;) {
        s_getcwd(pathbuf, sizeof(pathbuf));

        int nents;
        file_ent *list = file_list_create(".", &nents, image_file_filter);

        if (nents == 1 && strcmp(pathbuf, "/") && !list[0].isdir)
            launch_game(list[0].name);

        file_list_sort(list, nents);
        char namebuf[32];
        strcpy(namebuf, "Satiator - ");
        strlcat(namebuf, pathbuf, sizeof(namebuf));
        int entry = menu_picklist(list, nents, namebuf, NULL);
        int ret;
        if (entry == -1)
            s_chdir("..");
        else if (list[entry].isdir) {
            // Got to strip the slash :(
            list[entry].name[strlen(list[entry].name) - 1] = '\0';
            ret = s_chdir(list[entry].name);
            if (ret != FR_OK) {
                sprintf(test_buf, "Error %d ch '%s'", ret, list[entry].name);
                menu_error("chdir", test_buf);
            }
        }
        else
            name = strdup(list[entry].name);
        file_list_free(list, nents);

        if (name) {
            launch_game(name);
            free(name);
            name = NULL;
        }
    }
}
