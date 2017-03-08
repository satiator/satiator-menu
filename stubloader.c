#include "satisfier.h"
#include <string.h>
#include <iapetus.h>

extern char _ram_start, _ram_end, _ram_origin, _bss_end;
extern char _bios_hook_start, _bios_hook_end, _bios_hook_origin;

__attribute__ ((section(".bios_hook")))
void back_to_menu(void) {
    smpc_wait_till_ready();
    smpc_issue_command(SMPC_CMD_SSHOFF);
    smpc_wait_till_ready();

    asm volatile (
        "mov    #0xf0, r0   \n\t"
        "ldc    r0, sr      \n\t"
        : : : "r0"
    );

    // knock
    CDB_REG_HIRQ = ~HIRQ_EFLS;
    CDB_REG_CR1 = 0xe000;
    CDB_REG_CR2 = 0x0000;
    CDB_REG_CR3 = 0x00c1;
    CDB_REG_CR4 = 0x05e7;
    while (!(CDB_REG_HIRQ & HIRQ_EFLS));

    // bugger off back to menuland
    bios_get_mpeg_rom(2, 2, 0x200000);
    ((void(*)(void))0x200000)();
    for(;;);
}

void boot_disc(void) {
    // XXX stub to make linker happy
}

void load_ip_1st_read(uint8_t *dst) {
    cd_cmd_struct cmd;
    cd_stat_struct stat;

    cd_set_sector_size(SECT_2048);

    // Change Directory to root
    cmd.CR1 = 0x7000;
    cmd.CR2 = 0x0000;
    cmd.CR3 = 0x00ff;
    cmd.CR4 = 0xffff;
    cd_exec_command(HIRQ_EFLS, &cmd, &stat);
    cd_wait_hirq(HIRQ_EFLS);

    // read file #2
    cmd.CR1 = 0x7400;
    cmd.CR2 = 0x0000;
    cmd.CR3 = 0x0000;
    cmd.CR4 = 0x0002;
    cd_exec_command(HIRQ_EFLS, &cmd, &stat);

    int done = 0;
    while (!done) {
        if (CDB_REG_HIRQ & HIRQ_EFLS)
            done = 1;

        int nsec = cd_is_data_ready(0);
        if (!nsec)
            continue;
        cd_transfer_data_bytes(2048 * nsec, dst);
        dst += 2048*nsec;
    }
}

void main(void) {
    memset(&_ram_end, 0, &_bss_end-&_ram_end);

    volatile uint32_t *cdplayer = 0x0600026C;
    *cdplayer = &back_to_menu;

    s_mode(S_MODE_USBFS);

    s_emulate("out.desc");

    /* while (is_cd_present()); */
    /* while (!is_cd_present()); */
    s_mode(S_MODE_CDROM);

    // read whatever IP there be
    volatile int ret = cd_read_sector((void*)0x06002000, 150, SECT_2048, 16*2048);

    // read second file (if nec.)
    void *ip_load_addr = *(void**)0x060020f0;
    if (ip_load_addr)
        load_ip_1st_read(ip_load_addr);
}

 __attribute__((section(".start")))
void start(void) {
    memcpy(&_bios_hook_start, &_bios_hook_origin, &_bios_hook_end-&_bios_hook_start);
    // relocate to LoRAM
    memcpy(&_ram_start, &_ram_origin, &_ram_end-&_ram_start + &_bios_hook_end-&_bios_hook_start);
    main();

}
