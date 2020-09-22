#include <iapetus.h>
#include <satiator.h>

static int bcd2num(uint8_t bcd) {
    return (bcd >> 4)*10 + (bcd & 0xf);
}

void set_satiator_rtc(void) {
    smpc_wait_till_ready();

    SMPC_REG_IREG(0) = 0x01;
    SMPC_REG_IREG(1) = 0x00;
    SMPC_REG_IREG(2) = 0xF0; // ???
    smpc_issue_command(SMPC_CMD_INTBACK);

    smpc_wait_till_ready();

    int year = 100*bcd2num(SMPC_REG_OREG(1)) + bcd2num(SMPC_REG_OREG(2));
    int month = SMPC_REG_OREG(3) & 0xf;
    int day  = bcd2num(SMPC_REG_OREG(4));
    int hour = bcd2num(SMPC_REG_OREG(5));
    int min  = bcd2num(SMPC_REG_OREG(6));
    int sec  = bcd2num(SMPC_REG_OREG(7));

    uint32_t fattime =
          ((year - 1980) << 25)
        | (month << 21)
        | (day << 16)
        | (hour << 11)
        | (min << 5)
        | (sec >> 1);
    s_settime(fattime);
}
