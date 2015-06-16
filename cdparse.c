/*  Copyright 2015 James Laird-Wah
    Copyright 2004-2008, 2013 Theo Berkau
    Copyright 2005 Joost Peters
    Copyright 2005-2006 Guillaume Duhamel
    
    This file is part of Saturn Satisfier, and was derived from Yabause's
    cdbase.c at version 0.9.14. As such, it is licensed under the GPL,
    version >=2.
*/

// XXX: there are lots of memory leaks of track->filename in error paths

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "satisfier.h"

typedef  uint8_t   u8;
typedef  uint16_t  u16;
typedef  uint32_t  u32;
typedef  uint64_t  u64;
typedef  int8_t    s8;
typedef  int16_t   s16;
typedef  int32_t   s32;
typedef  int64_t   s64;

const char *cdparse_error_string = NULL;

#define stricmp strcasecmp
#define YabSetError(code, string) do { cdparse_error_string = string; } while(0)

typedef struct
{
   u8 ctl_addr;
   u32 fad_start;
   u32 fad_end;
   u32 file_offset;
   u32 sector_size;
   int file_size;
   int file_id;
   int interleaved_sub;
   int pregap;
   char *filename;
} track_info_struct;

typedef struct
{
   u32 fad_start;
   u32 fad_end;
   track_info_struct *track;
   int track_num;
} session_info_struct;

typedef struct
{
   int session_num;
   session_info_struct *session;
} disc_info_struct;

#pragma pack(push, 1)
typedef struct
{
   u8 signature[16];
   u8 version[2];
   u16 medium_type;
   u16 session_count;
   u16 unused1[2];
   u16 bca_length;
   u32 unused2[2];
   u32 bca_offset;
   u32 unused3[6];
   u32 disk_struct_offset;
   u32 unused4[3];
   u32 sessions_blocks_offset;
   u32 dpm_blocks_offset;
   u32 enc_key_offset;
} mds_header_struct;

typedef struct
{
   s32 session_start;
   s32 session_end;
   u16 session_number;
   u8 total_blocks;
   u8 leadin_blocks;
   u16 first_track;
   u16 last_track;
   u32 unused;
   u32 track_blocks_offset;
} mds_session_struct;

typedef struct
{
   u8 mode;
   u8 subchannel_mode;
   u8 addr_ctl;
   u8 unused1;
   u8 track_num;
   u32 unused2;
   u8 m;
   u8 s;
   u8 f;
   u32 extra_offset;
   u16 sector_size;
   u8 unused3[18];
   u32 start_sector;
   u64 start_offset;
   u8 session;
   u8 unused4[3];
   u32 footer_offset;
   u8 unused5[24];
} mds_track_struct;

typedef struct
{
   u32 filename_offset;
   u32 is_widechar;
   u32 unused1;
   u32 unused2;
} mds_footer_struct;

#pragma pack(pop)

enum IMG_TYPE { IMG_NONE, IMG_ISO, IMG_BINCUE, IMG_MDS, IMG_CCD, IMG_NRG };
enum IMG_TYPE imgtype = IMG_ISO;
static disc_info_struct disc;

#define MSF_TO_FAD(m,s,f) ((m * 4500) + (s * 75) + f)

//////////////////////////////////////////////////////////////////////////////

static int LoadBinCue(const char *cuefilename, FILE *iso_file)
{
   u32 size;
   char *temp_buffer, *temp_buffer2;
   unsigned int track_num;
   unsigned int indexnum, min, sec, frame;
   unsigned int pregap=0;
   char *p, *p2;
   track_info_struct trk[100];
   int file_size;
   int i;
   FILE * bin_file;

   disc.session_num = 1;
   disc.session = malloc(sizeof(session_info_struct) * disc.session_num);
   if (disc.session == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      return -1;
   }

   fseek(iso_file, 0, SEEK_END);
   size = ftell(iso_file);
   fseek(iso_file, 0, SEEK_SET);

   // Allocate buffer with enough space for reading cue
   if ((temp_buffer = (char *)calloc(size, 1)) == NULL)
      return -1;

   // Skip image filename
   if (fscanf(iso_file, "FILE \"%*[^\"]\" %*s\r\n") == EOF)
   {
      free(temp_buffer);
      return -1;
   }

   // Time to generate TOC
   for (;;)
   {
      // Retrieve a line in cue
      if (fscanf(iso_file, "%s", temp_buffer) == EOF)
         break;

      // Figure out what it is
      if (strncmp(temp_buffer, "TRACK", 5) == 0)
      {
         // Handle accordingly
         if (fscanf(iso_file, "%d %[^\r\n]\r\n", &track_num, temp_buffer) == EOF)
            break;

         if (strncmp(temp_buffer, "MODE1", 5) == 0 ||
            strncmp(temp_buffer, "MODE2", 5) == 0)
         {
            // Figure out the track sector size
            trk[track_num-1].sector_size = atoi(temp_buffer + 6);
            trk[track_num-1].ctl_addr = 0x41;
         }
         else if (strncmp(temp_buffer, "AUDIO", 5) == 0)
         {
            // Update toc entry
            trk[track_num-1].sector_size = 2352;
            trk[track_num-1].ctl_addr = 0x01;
         }
      }
      else if (strncmp(temp_buffer, "INDEX", 5) == 0)
      {
         // Handle accordingly

         if (fscanf(iso_file, "%d %d:%d:%d\r\n", &indexnum, &min, &sec, &frame) == EOF)
            break;

         if (indexnum == 1)
         {
            // Update toc entry
            trk[track_num-1].fad_start = (MSF_TO_FAD(min, sec, frame) + pregap + 150);
            trk[track_num-1].file_offset = MSF_TO_FAD(min, sec, frame) * trk[track_num-1].sector_size;
            trk[track_num-1].pregap = pregap;
         }
      }
      else if (strncmp(temp_buffer, "PREGAP", 6) == 0)
      {
         if (fscanf(iso_file, "%d:%d:%d\r\n", &min, &sec, &frame) == EOF)
            break;

         trk[track_num-1].pregap = MSF_TO_FAD(min, sec, frame);
      }
      else if (strncmp(temp_buffer, "POSTGAP", 7) == 0)
      {
         if (fscanf(iso_file, "%d:%d:%d\r\n", &min, &sec, &frame) == EOF)
            break;
      }
      else if (strncmp(temp_buffer, "FILE", 4) == 0)
      {
         YabSetError(YAB_ERR_OTHER, "Unsupported cue format");
         free(temp_buffer);
         return -1;
      }
   }

   trk[0].pregap = 150;
   trk[track_num].file_offset = 0;
   trk[track_num].fad_start = 0xFFFFFFFF;

   // Go back, retrieve image filename
   fseek(iso_file, 0, SEEK_SET);
   fscanf(iso_file, "FILE \"%[^\"]\" %*s\r\n", temp_buffer);

   char *bin_filename = temp_buffer;

   // Now go and open up the image file, figure out its size, etc.
   if ((bin_file = fopen(bin_filename, "rb")) == NULL)
   {
      // Ok, exact path didn't work. Let's trim the path and try opening the
      // file from the same directory as the cue.

      // find the start of filename
      p = temp_buffer;

      for (;;)
      {
         if (strcspn(p, "/\\") == strlen(p))
         break;

         p += strcspn(p, "/\\") + 1;
      }

      // append directory of cue file with bin filename
      if ((temp_buffer2 = (char *)calloc(strlen(cuefilename) + strlen(p) + 1, 1)) == NULL)
      {
         free(temp_buffer);
         return -1;
      }

      // find end of path
      p2 = (char *)cuefilename;

      for (;;)
      {
         if (strcspn(p2, "/\\") == strlen(p2))
            break;
         p2 += strcspn(p2, "/\\") + 1;
      }

      // Make sure there was at least some kind of path, otherwise our
      // second check is pretty useless
      if (cuefilename == p2 && temp_buffer == p)
      {
         free(temp_buffer);
         free(temp_buffer2);
         return -1;
      }

      strncpy(temp_buffer2, cuefilename, p2 - cuefilename);
      strcat(temp_buffer2, p);

      // Let's give it another try
      bin_filename = temp_buffer2;
      bin_file = fopen(bin_filename, "rb");
      free(temp_buffer);

      if (bin_file == NULL)
      {
         YabSetError(YAB_ERR_FILENOTFOUND, bin_filename);
         free(bin_filename);
         return -1;
      }
   }

   fseek(bin_file, 0, SEEK_END);
   file_size = ftell(bin_file);
   fseek(bin_file, 0, SEEK_SET);

   for (i = 0; i < track_num; i++)
   {
      trk[i].fad_end = trk[i+1].fad_start-1;
      trk[i].file_id = 0;
      trk[i].file_size = file_size;
      trk[i].filename = strdup(bin_filename);
   }
   free(bin_filename);

   trk[track_num-1].fad_end = trk[track_num-1].fad_start+(file_size-trk[track_num-1].file_offset)/trk[track_num-1].sector_size;

   disc.session[0].fad_start = 150;
   disc.session[0].fad_end = trk[track_num-1].fad_end;
   disc.session[0].track_num = track_num;
   disc.session[0].track = malloc(sizeof(track_info_struct) * disc.session[0].track_num);
   if (disc.session[0].track == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      free(disc.session);
      disc.session = NULL;
      return -1;
   }

   memcpy(disc.session[0].track, trk, track_num * sizeof(track_info_struct));

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int LoadMDSTracks(const char *mds_filename, FILE *iso_file, mds_session_struct *mds_session, session_info_struct *session)
{
   int i;
   int track_num=0;
   u32 fad_end;

   session->track = malloc(sizeof(mds_track_struct) * mds_session->last_track);
   if (session->track == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      return -1;
   }

   for (i = 0; i < mds_session->total_blocks; i++)
   {
      mds_track_struct track;
      FILE *fp=NULL;
      char filename[512];
      int file_size = 0;

      fseek(iso_file, mds_session->track_blocks_offset + i * sizeof(mds_track_struct), SEEK_SET);
      if (fread(&track, 1, sizeof(mds_track_struct), iso_file) != sizeof(mds_track_struct))
      {
         YabSetError(YAB_ERR_FILEREAD, mds_filename);
         free(session->track);
         return -1;
      }

      if (track.track_num == 0xA2)
         fad_end = MSF_TO_FAD(track.m, track.s, track.f);
      if (!track.extra_offset)
         continue;

      if (track.footer_offset)
      {
         mds_footer_struct footer;
         int found_dupe=0;
         int j;

         // Make sure we haven't already opened file already
         for (j = 0; j < track_num; j++)
         {
            if (track.footer_offset == session->track[j].file_id)
            {
               found_dupe = 1;
               break;
            }
         }

         if (found_dupe)
         {
            strcpy(filename, session->track[j].filename);
            file_size = session->track[j].file_size;
         }
         else
         {
            fseek(iso_file, track.footer_offset, SEEK_SET);
            if (fread(&footer, 1, sizeof(mds_footer_struct), iso_file) != sizeof(mds_footer_struct))
            {
               YabSetError(YAB_ERR_FILEREAD, mds_filename);
               free(session->track);
               return -1;
            }

            fseek(iso_file, footer.filename_offset, SEEK_SET);
            if (footer.is_widechar)
            {
                // we don't support wide chars on FAT
                return -1;
            }
            char img_filename[512];
            memset(img_filename, 0, 512);

            if (fscanf(iso_file, "%512c", img_filename) != 1)
            {
               YabSetError(YAB_ERR_FILEREAD, mds_filename);
               free(session->track);
               return -1;
            }

            if (strncmp(img_filename, "*.", 2) == 0)
            {
               char *ext;
               strcpy(filename, mds_filename);
               ext = strrchr(filename, '.');
               strcpy(ext, img_filename+1);
            }
            else
               strcpy(filename, img_filename);

            fp = fopen(filename, "rb");

            if (fp == NULL)
            {
               YabSetError(YAB_ERR_FILEREAD, filename);
               free(session->track);
               return -1;
            }

            fseek(fp, 0, SEEK_END);
            file_size = ftell(fp);
            fclose(fp);
         }
      }

      session->track[track_num].ctl_addr = (((track.addr_ctl << 4) | (track.addr_ctl >> 4)) & 0xFF);
      session->track[track_num].fad_start = track.start_sector+150;
      session->track[track_num].pregap = 150;   // can MDS do non-150 pregap? surely
      if (track_num > 0)
         session->track[track_num-1].fad_end = session->track[track_num].fad_start;
      session->track[track_num].file_offset = track.start_offset;
      session->track[track_num].sector_size = track.sector_size;
      session->track[track_num].filename = strdup(filename);
      session->track[track_num].file_size = file_size;
      session->track[track_num].file_id = track.footer_offset;
      session->track[track_num].interleaved_sub = track.subchannel_mode != 0 ? 1 : 0;

      track_num++;
   }

   session->track[track_num-1].fad_end = fad_end;
   session->fad_start = session->track[0].fad_start;
   session->fad_end = fad_end;
   session->track_num = track_num;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int LoadMDS(const char *mds_filename, FILE *iso_file)
{
   s32 i;
   mds_header_struct header;

   fseek(iso_file, 0, SEEK_SET);

   if (fread((void *)&header, 1, sizeof(mds_header_struct), iso_file) != sizeof(mds_header_struct))
   {
      YabSetError(YAB_ERR_FILEREAD, "Can't read header");
      return -1;
   }
   else if (memcmp(&header.signature,  "MEDIA DESCRIPTOR", sizeof(header.signature)))
   {
      YabSetError(YAB_ERR_OTHER, "Bad MDS header");
      return -1;
   }
   else if (header.version[0] > 1)
   {
      YabSetError(YAB_ERR_OTHER, "Unsupported MDS version");
      return -1;
   }

   if (header.medium_type & 0x10)
   {
      // DVD's aren't supported, not will they ever be
      YabSetError(YAB_ERR_OTHER, "DVD's aren't supported");
      return -1;
   }

   disc.session_num = header.session_count;
   disc.session = malloc(sizeof(session_info_struct) * disc.session_num);
   if (disc.session == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      return -1;
   }

   for (i = 0; i < header.session_count; i++)
   {
      mds_session_struct session;

      fseek(iso_file, header.sessions_blocks_offset + i * sizeof(mds_session_struct), SEEK_SET);
      if (fread(&session, 1, sizeof(mds_session_struct), iso_file) != sizeof(mds_session_struct))
      {
         free(disc.session);
         YabSetError(YAB_ERR_FILEREAD, "Read error");
         return -1;
      }

      if (LoadMDSTracks(mds_filename, iso_file, &session, &disc.session[i]) != 0)
         return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int LoadISO(const char *isoname, FILE *iso_file)
{
   track_info_struct *track;
   dprintf("LoadISO\n");

   disc.session_num = 1;
   disc.session = malloc(sizeof(session_info_struct) * disc.session_num);
   if (disc.session == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      return -1;
   }

   disc.session[0].fad_start = 150;
   disc.session[0].track_num = 1;
   disc.session[0].track = malloc(sizeof(track_info_struct) * disc.session[0].track_num);
   if (disc.session[0].track == NULL)
   {
      YabSetError(YAB_ERR_MEMORYALLOC, NULL);
      free(disc.session);
      disc.session = NULL;
      return -1;
   }

   track = disc.session[0].track;
   track->ctl_addr = 0x41;
   track->fad_start = 150;
   track->pregap = 150;
   track->file_offset = 0;
   track->filename = strdup(isoname);
   fseek(iso_file, 0, SEEK_END);
   track->file_size = ftell(iso_file);
   track->file_id = 0;

   if (0 == (track->file_size % 2048))
      track->sector_size = 2048;
   else if (0 == (track->file_size % 2352))
      track->sector_size = 2352;
   else
   {
      YabSetError(YAB_ERR_OTHER, "Unsupported CD image!\n");
      return -1;
   }

   dprintf("LoadISO done\n");
   disc.session[0].fad_end = track->fad_end = disc.session[0].fad_start + (track->file_size / track->sector_size);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int BuildDesc()
{
   FILE *descfile = fopen("out.desc", "wb");
   satisfier_trackdesc_t desc;
   session_info_struct *session=&disc.session[0];
   dprintf("BuildDesc\n");

   int i;
   for (i = 0; i < session->track_num; i++)
   {
      dprintf("  track %d\n", i+1);
      memset(&desc, 0, sizeof(desc));
      track_info_struct *track=&disc.session[0].track[i];
      desc.number = i+1;
      desc.toc_ent = (track->ctl_addr << 24) | track->fad_start;
      desc.pregap = track->pregap;
      desc.file_offset = track->file_offset;
      switch(track->sector_size) {
         case 2048:
            desc.file_secsize = SEC_2048;
            break;
         case 2352:
            desc.file_secsize = SEC_2352;
            break;
         case 2448:
            desc.file_secsize = SEC_2448;
            break;
         default:
            return -1;
      }
      desc.namelen = strlen(track->filename);
      fwrite(&desc, offsetof(satisfier_trackdesc_t, name), 1, descfile);
      fwrite(track->filename, desc.namelen, 1, descfile);
   }

   uint8_t ctl_addr;
   memset(&desc, 0, sizeof(desc));
   desc.number = 100;    // first info track
   ctl_addr = session[0].track[0].ctl_addr; 
   desc.toc_ent = (ctl_addr << 24) | MSF_TO_FAD(1, 0, 0);
   fwrite(&desc, offsetof(satisfier_trackdesc_t, name), 1, descfile);

   desc.number = 101;   // last info track
   ctl_addr = session[0].track[session[0].track_num-1].ctl_addr;
   desc.toc_ent = (ctl_addr << 24) | MSF_TO_FAD(session[0].track_num, 0, 0);
   fwrite(&desc, offsetof(satisfier_trackdesc_t, name), 1, descfile);

   desc.number = 102;   // leadout
   desc.toc_ent = (ctl_addr << 24) | session[0].fad_end;
   fwrite(&desc, offsetof(satisfier_trackdesc_t, name), 1, descfile);

   memset(&desc, 0, sizeof(desc));  // terminator
   fwrite(&desc, offsetof(satisfier_trackdesc_t, name), 1, descfile);

   dprintf("Closing descfile\n");
   fclose(descfile);
   dprintf("Done with descfile\n");

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int ISOCDInit(const char * iso) {
   char header[6];
   char *ext;
   int ret;
   FILE *iso_file;

   dprintf("ISOCDInit(\"%s\")\n", iso);

   memset(&disc, 0, sizeof(disc));
   dprintf("cleared disc struct\n");

   if (!iso)
      return -1;

   if (!(iso_file = fopen(iso, "rb")))
   {
      YabSetError(YAB_ERR_FILENOTFOUND, "File not found");
      return -1;
   }
   dprintf("opened input\n");

   fread((void *)header, 1, 6, iso_file);
   dprintf("read header\n");
   ext = strrchr(iso, '.');
   
   dprintf("ext = \"%s\"\n", ext);

   // Figure out what kind of image format we're dealing with
   if (stricmp(ext, ".CUE") == 0 && strncmp(header, "FILE \"", 6) == 0)
   {
      // It's a BIN/CUE
      imgtype = IMG_BINCUE;
      ret = LoadBinCue(iso, iso_file);
   }
   else if (stricmp(ext, ".MDS") == 0 && strncmp(header, "MEDIA ", sizeof(header)) == 0)
   {
      // It's a MDS
      imgtype = IMG_MDS;
      ret = LoadMDS(iso, iso_file);
   }
   else if (stricmp(ext, ".ISO") == 0)
   {
      // It's an ISO
      imgtype = IMG_ISO;
      ret = LoadISO(iso, iso_file);
   } else
   {
       YabSetError(0, "Unrecognised extension");
       return -1;
   }

   if (ret != 0)
   {
      imgtype = IMG_NONE;
      return -1;
   }   

   BuildDesc();
   dprintf("load complete\n");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void ISOCDDeInit(void) {
   int i, j, k;
   if (disc.session)
   {
      for (i = 0; i < disc.session_num; i++)
      {
         if (disc.session[i].track)
         {
            for (j = 0; j < disc.session[i].track_num; j++)
            {
               if (disc.session[i].track[j].filename)
               {
                  free(disc.session[i].track[j].filename);
               }
            }
            free(disc.session[i].track);
         }
      }
      free(disc.session);
   }
}

int load_iso(const char *filename) {
   return ISOCDInit(filename);
}

// vim: ts=3 sts=3 sw=3
