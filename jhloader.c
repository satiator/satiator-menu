/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include <iapetus.h>
#include "jhloader.h"

static int emulate_bios_loadcd_init(void) {
   *(uint32_t*)0x6000278 = 0;
   *(uint32_t*)0x600027c = 0;
   cd_abort_file();
   cd_end_transfer();
   cd_reset_selector_all();
   cd_set_sector_size(SECT_2048);
   cd_auth();   // gotta make sure that's a real disc thar
   return 0;
}

static struct region_s {
   uint8_t id;
   const char *key;
} regions[] = {
   {1,   "JAPAN"},
   {2,   "TAIWAN and PHILIPINES"},
   {4,   "USA and CANADA"},
   {5,   "BRAZIL"},
   {6,   "KOREA"},
   {10,  "ASIA PAL area"},
   {12,  "EUROPE"},
   {13,  "LATIN AMERICA"},
   {0,   NULL}
};

const char *get_region_string(void) {
   // fetch SMPC region byte
   smpc_wait_till_ready();
   SMPC_REG_IREG(0) = 0x01;
   SMPC_REG_IREG(1) = 0x00;
   SMPC_REG_IREG(2) = 0xf0;
   smpc_issue_command(SMPC_CMD_INTBACK);
   smpc_wait_till_ready();

   uint8_t region = SMPC_REG_OREG(9);
   struct region_s *r;
   for (r=regions; r->id; r++)
      if (r->id == region)
         return r->key;
   return NULL;
}

static int set_image_region(u8 *base)
{
   // set region header
   memset(base + 0x40, ' ', 0x10);
   base[0x40] = region_string[0];

   // set region footer
   char *ptr = (char*)base + 0xe00;
   memset(ptr, ' ', 0x20);
   *(uint32_t*)ptr = 0xa00e0009;
   strcpy(ptr + 4, "For ");
   strcpy(ptr + 8, region_string);
   char *end = ptr + 4 + strlen(ptr + 4);
   *end = '.';
   return 0;
}

static int emulate_bios_loadcd_read(void)
{
   int ret, i;

   // doesn't matter where
   u8 *ptr = (u8*)0x6002000;

   ret = cd_read_sector(ptr, 150, SECT_2048, 2048*16);
   if (ret < 0)
      return ret;
   
   ret = set_image_region(ptr);
   if (ret < 0)
      return ret;

   ret = cd_put_sector_data(0, 16);
   if (ret < 0)
      return ret;
   while (!(CDB_REG_HIRQ & HIRQ_DRDY)) {}

   for (i = 0; i < 2048 * 16; i+=4)
      CDB_REG_DATATRNS = *(uint32_t*)(ptr + i);

   if ((ret = cd_end_transfer()) != 0)
      return ret;

   while (!(CDB_REG_HIRQ & HIRQ_EHST)) {}

   // we need to set a flag, but it's in different places in different BIOS versions
   uint16_t *read_flag_a = (uint16_t*)0x06000380;
   uint16_t *read_flag_b = (uint16_t*)0x060003a0;

   uint16_t **read_flag_ptr_a = (uint16_t**)0x2174;
   uint16_t **read_flag_ptr_b = (uint16_t**)0x2a04;

   if (read_flag_a == *read_flag_ptr_a)
      *read_flag_a = 1;
   else if (read_flag_b == *read_flag_ptr_b)
      *read_flag_b = 1;
   else
       return BOOT_UNRECOGNISED_BIOS;

   return 0;
}


int boot_disc(void)
{
   int ret;
#if 0
   // authentic boot
   ret = bios_loadcd_init();
   ret = bios_loadcd_read();
#else
   // region free boot
   if ((ret = emulate_bios_loadcd_init()) < 0)
       return ret;
   if ((ret = emulate_bios_loadcd_read()) < 0)
       return ret;
#endif
   do {
       ret = bios_loadcd_boot();
       vdp_vsync();
   } while (ret == 1);
   return ret;
}
