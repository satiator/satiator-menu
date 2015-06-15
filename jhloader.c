/*  Copyright (c) 2015 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include <iapetus.h>

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
   char id;
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

static const char *get_region_string(void) {
   char region = *(volatile char*)0x20100033;
   struct region_s *r;
   for (r=regions; r->id; r++)
      if (r->id == region)
         return r->key;
   return NULL;
}

static int set_image_region(u8 *base)
{
   const char *str = get_region_string();
   if (!str)
      return -1;

   // set region header
   memset(base + 0x40, ' ', 0x10);
   base[0x40] = str[0];

   // set region footer
   char *ptr = (char*)base + 0xe00;
   memset(ptr, ' ', 0x20);
   *(uint32_t*)ptr = 0xa00e0009;
   strcpy(ptr + 4, "For ");
   strcpy(ptr + 8, str);
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

   *(uint16_t*)0x60003a0 = 1;

   return 0;
}


int boot_disc(void)
{
   int ret;

#if 0
   // authentic boot
   bios_loadcd_init();
   bios_loadcd_read();
#else
   // region free boot
   if ((ret = emulate_bios_loadcd_init()) < 0)
       return ret;
   if ((ret = emulate_bios_loadcd_read()) < 0)
       return ret;
#endif
   while (cd_is_data_ready(0) < 16)
       vdp_vsync();
   ret = bios_loadcd_boot();
   return ret;
}
