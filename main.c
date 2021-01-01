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
        case BOOT_UNRECOGNISED_BIOS:
            error = "Unrecognised BIOS version! Contact Abrasive info@satiator.net";
            break;
    }
    menu_error("Boot failed!", error);
}

char pathbuf[512];
void image_menu(void) {
    char *name = NULL;
    strcpy(pathbuf, "/");

    for(;;) {
        int nents;
        file_ent *list = file_list_create(".", &nents, image_file_filter);

        if (nents == 1 && strcmp(pathbuf, "/") && !list[0].isdir)
            launch_game(list[0].name);

        file_list_sort(list, nents);
        char namebuf[32];
        strcpy(namebuf, "Satiator - ");
        char *dirname = strrchr(pathbuf, '/');
        if (dirname[1])
            dirname++;
        strlcat(namebuf, dirname, sizeof(namebuf));
        int entry = menu_picklist(list, nents, namebuf);
        int ret;
        if (entry == -1) {
            if (!strcmp(pathbuf, "/")) {
                return;
            } else {
                char *last_slash = strrchr(pathbuf, '/');
                if (last_slash == pathbuf)
                    last_slash++;
                *last_slash = '\0';
                s_chdir(pathbuf);
            }
        } else if (list[entry].isdir) {
            // Got to strip the slash :(
            list[entry].name[strlen(list[entry].name) - 1] = '\0';
            if (pathbuf[1])
                strcat(pathbuf, "/");
            strcat(pathbuf, list[entry].name);
            ret = s_chdir(pathbuf);
            if (ret != FR_OK) {
                menu_error("chdir", "Can't change directory, corrupt SD card?");
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

void ar_menu(void);
const file_ent top_menu_options[] = {
    {"Browse images", 0, &image_menu},
    {"Action Replay tools", 0, &ar_menu},
};

void main_menu(void) {
    menu_init();
    image_menu();

    while (1) {
        int entry = menu_picklist(top_menu_options, sizeof(top_menu_options)/sizeof(*top_menu_options), "Satiator");
        if (entry == -1)
            continue;

        void (*submenu)(void) = top_menu_options[entry].priv;
        submenu();
    }
}
