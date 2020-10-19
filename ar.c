#include <satiator.h>
#include <iapetus.h>
#include <string.h>
#include <stdio.h>
#include "gui/gmenu.h"

#define AR_FLASH_ADDR   ((volatile u16 *)0x22000000)
static flash_info_struct flash_info;
int flash_inited = 0;

static int init_flash(void) {
    if (flash_inited)
        return 1;

    int ret = ar_init_flash_io(&flash_info);
    if (ret != IAPETUS_ERR_OK) {
        menu_error("AR cartridge", "Couldn't initialise flash IO, is the cart seated properly?");
        return 0;
    }

    flash_inited = 1;
    return 1;
}

static void flash_backup_ar(void) {
    if (!init_flash())
        return;

    int fd = s_open("ar_backup.bin", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    if (fd < 0) {
        menu_error("Action Replay", "Could not open ar_backup.bin for writing");
        return;
    }

    int length = flash_info.page_size * flash_info.num_pages;
    int done = 0;
    uint8_t *ptr = (void*)AR_FLASH_ADDR;

    menu_progress_begin("Reading...", length);

    while (done < length) {
        int to_copy = length;
        if (to_copy > S_MAXBUF)
            to_copy = S_MAXBUF;
        s_write(fd, ptr, to_copy);
        done += to_copy;
        ptr += to_copy;
        menu_progress_update(done);
        for (volatile int i=0; i<10000; i++);
    }

    menu_progress_complete();

    s_close(fd);
    menu_error("Action Replay", "Successfully backed up to ar_backup.bin.");
}

static void flash_erase_ar(void) {
    if (!init_flash())
        return;

    menu_progress_begin("Erasing...", flash_info.num_pages);

    uint16_t *erase_ptr = (void*)AR_FLASH_ADDR;

    for (int page=0; page<flash_info.num_pages; page++) {
        menu_progress_update(page);

        int is_clear = 1;
        for (int word=0; word<flash_info.page_size/2; word++) {
            if (erase_ptr[word] != 0xffff) {
                is_clear = 0;
                break;
            }
        }
        if (!is_clear)
            ar_erase_flash(&flash_info, erase_ptr, 1);

        erase_ptr += flash_info.page_size/2;
    }
    menu_progress_complete();
}


static int bin_file_filter(file_ent *entry) {
    if (entry->isdir)
        return 1;

    int len = strlen(entry->name);
    if (!strcasecmp(&entry->name[len-4], ".bin"))
        return 1;

    return 0;
}

static void flash_flash_ar(void) {
    if (!init_flash())
        return;

    int nents;
    file_ent *list = file_list_create(".", &nents, bin_file_filter);
    file_list_sort(list, nents);

    int entry = menu_picklist(list, nents, "Choose image to flash");
    if (entry < 0)
        return;

    int fd = s_open(list[entry].name, FA_READ);
    file_list_free(list, nents);

    if (fd < 0) {
        menu_error("Action Replay", "Could not open file");
        return;
    }

    int file_size = s_seek(fd, 0, 2);
    s_seek(fd, 0, 0);


    uint8_t buf[S_MAXBUF], empty[S_MAXBUF];
    int write_size = S_MAXBUF;
    int blocks = (file_size + write_size - 1) / write_size;
    int pages_per = write_size / flash_info.page_size;
    int page_words = write_size / 2;
    volatile u16 *write_ptr = AR_FLASH_ADDR;

    memset(empty, 0xff, sizeof(empty));

    menu_progress_begin("Writing...", blocks);

    for (int i=0; i<blocks; i++) {
        menu_progress_update(i);

        s_read(fd, buf, write_size);

        if (memcmp((void*)write_ptr, buf, page_words*2)) {
            if (memcmp((void*)write_ptr, empty, page_words*2))
                ar_erase_flash(&flash_info, write_ptr, pages_per);
            ar_write_flash(&flash_info, write_ptr, (void*)buf, pages_per);

            if (memcmp((void*)write_ptr, buf, page_words*2)) {
                char msg_buf[64];
                sprintf(msg_buf, "Verify error on block %d", i);
                menu_error("Action Replay", msg_buf);
                return;
            }
        }

        write_ptr += page_words;
    }
    menu_progress_complete();

    menu_error("Action Replay", "Flashing complete");
}

const file_ent ar_menu_options[] = {
    {"Backup AR", 0, &flash_backup_ar},
    {"Erase AR", 0, &flash_erase_ar},
    {"Flash AR", 1, &flash_flash_ar},
};

void ar_menu(void) {
    while (1) {
        int entry = menu_picklist(ar_menu_options, sizeof(ar_menu_options)/sizeof(*ar_menu_options), "Action Replay tools");
        if (entry == -1)
            return;

        void (*submenu)(void) = ar_menu_options[entry].priv;
        submenu();
    }
}
