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
    char msg_buf[64];
    uint16_t vid, pid;
    if (ret != IAPETUS_ERR_OK) {
        switch (ret) {
            case IAPETUS_ERR_UNSUPPORTED:
                ar_get_product_id(&vid, &pid);
                sprintf(msg_buf, "Unsupported Flash ID %04x:%04x", vid, pid);
                menu_error("Action Replay", msg_buf);
                return 0;

            case IAPETUS_ERR_HWNOTFOUND:
                menu_error("Action Replay", "No AR cart found!");
                return 0;

            default:
                menu_error("Action Replay", "Unknown error");
                return 0;
        }
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

    int length = flash_info.page_size * flash_info.num_pages * 2;
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
    ar_erase_flash_all(&flash_info);
    menu_progress_complete();

    menu_error("Action Replay", "Erasing successful.");
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

    if (file_size > (2*flash_info.page_size * flash_info.num_pages)) {
        menu_error("Action Replay", "Firmware too big for Flash");
        return;
    }

    int block_bytes = flash_info.page_size * 2;
    if (block_bytes < S_MAXBUF)
        block_bytes = S_MAXBUF;

    uint8_t buf[block_bytes], empty[block_bytes];
    int blocks = (file_size + block_bytes - 1) / block_bytes;
    int block_words = block_bytes / 2;
    int pages_per = block_words / flash_info.page_size;
    volatile u16 *write_ptr = AR_FLASH_ADDR;

    memset(empty, 0xff, sizeof(empty));

    menu_progress_begin("Writing...", blocks);

    for (int i=0; i<blocks; i++) {
        menu_progress_update(i);

        for (int nread = 0; nread < block_bytes; nread += S_MAXBUF)
            s_read(fd, &buf[nread], S_MAXBUF);

        int matches = 1;
        int empty = 1;
        uint16_t *bptr = (void*)buf;
        for (volatile uint16_t *rptr = write_ptr; rptr < write_ptr+block_words; rptr++) {
            if (*rptr != 0xffff)
                empty = 0;
            if (*rptr != *bptr++)
                matches = 0;
            if (!(empty | matches))
                break;
        }

        if (!matches) {
            if (flash_info.needs_page_erase && !empty)
                ar_erase_flash(&flash_info, write_ptr, pages_per);
            ar_write_flash(&flash_info, write_ptr, (void*)buf, pages_per);

            char msg_buf[64];
            for (int j=0; j<block_words; j++) {
                u16 got = write_ptr[j];
                u16 want = ((u16*)buf)[j];
                if (got != want) {
                    sprintf(msg_buf, "Verify error at address 0x%x: %04x!=%04x", j*2, got, want);
                    menu_error("Action Replay", msg_buf);
                    return;
                }
            }
        }

        write_ptr += block_words;
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
