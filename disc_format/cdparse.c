/*  Copyright (c) 2017 James Laird-Wah
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "cdparse.h"

const char *cdparse_error_string = NULL;

int image2desc(const char *infile, const char *outfile) {
   cdparse_error_string = NULL;

   const char *dot = strrchr(infile, '.');
   if (!dot) {
      cdparse_error_string = "Unrecognised file extension";
      return 1;
   }

   const char *extension = dot + 1;

   if (!strcasecmp(extension, "cue"))
      return cue2desc(infile, outfile);

   cdparse_error_string = "Unrecognised file extension";
   return 1;
}
