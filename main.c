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

void restore_vdp_mem(void);

void launch_game(const char *filename) {
    dbgprintf("Loading ISO: '%s'\n", filename);
    int ret = image2desc(filename, "out.desc");
    if (ret) {
        menu_error("Disc load failed!", cdparse_error_string ? cdparse_error_string : "Unknown error");
        return;
    }

    fadeout(0x20);

    satiator_cart_header_t *cart = s_find_cartridge();
    if (cart && cart->header_version >= 1)
        cart->install_soft_reset_hook();

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

const file_ent format_confirm_menu_options[] = {
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"No", 0, NULL},
    {"Yes", 0, 1},
};

extern uint8_t vdp1_stash[];

void format_confirm(int flags) {
    int entry = menu_picklist(format_confirm_menu_options, sizeof(format_confirm_menu_options)/sizeof(*format_confirm_menu_options), "Select format_confirm");
    if (entry == -1)
        return;
    if (!format_confirm_menu_options[entry].priv)
        return;

    menu_progress_begin("Formatting...", 3);

    int menu_size = 0;
    int fd = s_open("/menu.bin", FA_READ);
    int ret;
    uint8_t *p = vdp1_stash;
    while ((ret = s_read(fd, p, 2048)) > 0) {
        p += ret;
        menu_size += ret;
    }
    s_close(fd);
    menu_progress_update(1);

    int mkfs_ret = s_format_sd_card(flags);
    menu_progress_update(2);

    fd = s_open("/menu.bin", FA_WRITE|FA_CREATE_ALWAYS);
    p = vdp1_stash;
    while (menu_size > 0) {
        int count = menu_size;
        if (count > 2048)
            count = 2048;
        ret = s_write(fd, p, count);
        if (ret < 0)
            break;
        p += count;
        menu_size -= count;
    }
    s_close(fd);

    menu_progress_complete();

    char msg_buf[64];

    if (mkfs_ret != FR_OK)
        sprintf(msg_buf, "Error %d", mkfs_ret);
    else
        sprintf(msg_buf, "Success!");

    menu_error("SD formatting complete", msg_buf);
}

const file_ent format_menu_options[] = {
    {"FAT32", 0, &format_confirm, 1},
    {"exFAT", 0, &format_confirm, 0},
};

void format_menu(void) {
    int entry = menu_picklist(format_menu_options, sizeof(format_menu_options)/sizeof(*format_menu_options), "Select format");
    if (entry == -1)
        return;

    void (*submenu)(int) = format_menu_options[entry].priv;
    submenu(format_menu_options[entry].extra);
}

void ar_menu(void);
const file_ent top_menu_options[] = {
    {"Browse images", 0, &image_menu},
    {"Action Replay tools", 0, &ar_menu},
    {"Format SD card", 0, &format_menu},
};

void main_menu(void) {
    menu_init();
    s_chdir("\\");
    image_menu();

    while (1) {
        int entry = menu_picklist(top_menu_options, sizeof(top_menu_options)/sizeof(*top_menu_options), "Satiator");
        if (entry == -1)
            continue;

        void (*submenu)(void) = top_menu_options[entry].priv;
        submenu();
    }
}
