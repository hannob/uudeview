/*
 * This file is part of uudeview, the simple and friendly multi-part multi-
 * file uudecoder  program  (c) 1994-2001 by Frank Pilhofer. The author may
 * be contacted at fp@fpx.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Predefined Filename filters. They aren't part of the Library, because
 * they are system-dependent. Just add this file to your project. If you
 * write filters for other systems, or find problems with these here, let
 * me know.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef SYSTEM_WINDLL
#include <windows.h>
#endif
#ifdef SYSTEM_OS2
#include <os2.h>
#endif

#include <stdio.h>
#include <ctype.h>

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include <fptools.h>
#include <uufnflt.h>


char * uufnflt_id = "$Id: uufnflt.c,v 1.7 2003/04/13 15:41:54 fp Exp $";

char *
UUFNameFilterUnix (void *opaque, char *fname)
{
  char *ptr;

  if (fname == NULL)
    return NULL;

  /*
   * strip directory information
   */

  if ((ptr = _FP_strrchr (fname, '/')) != NULL) {
    fname = ptr+1;
  }

  if ((ptr = _FP_strrchr (fname, '\\')) != NULL) {
    fname = ptr+1;
  }

  if (strcmp (fname, ".") == 0 || strcmp (fname, "..") == 0) {
    fname[0] = '_';
  }

  /*
   * remove space characters from the beginning
   */

  while (*fname == ' ') {
    fname++;
  }

  /*
   * If filename is empty by now, return a dummy string
   */

  if (!*fname) {
    return "blank_filename";
  }

  /*
   * Replace all questionable punctuation characters by underscores
   */

  for (ptr=fname; *ptr; ptr++) {
    if (*ptr == '"' || *ptr == '<' || *ptr == '>' ||
	*ptr == '{' || *ptr == '}' || *ptr == '|' ||
	*ptr == '&' || *ptr == '\t' || *ptr == '\r' ||
	*ptr == '\n') {
      *ptr = '_';
    }
  }
  
  return fname;
}

char *
UUFNameFilterDOS (void *opaque, char *fname)
{
  static char dosname[13], *ptr1, *ptr2;
  int count=0;

  if (fname == NULL)
    return NULL;

  /*
   * strip directory information
   */

  if ((ptr1 = _FP_strrchr (fname, '/')) != NULL)
    fname = ptr1 + 1;
  else if ((ptr1 = _FP_strrchr (fname, '\\')) != NULL)
    fname = ptr1 + 1;

  ptr1 = dosname;

  while (*fname && *fname!='.' && count++ < 8) {
    if (*fname == ' ')
      *ptr1++ = '_';
    else
      *ptr1++ = *fname;
    fname++;
  }
  while (*fname && *fname!='.')
    fname++;
  if (ptr1 == dosname)
    *ptr1++ = '_';
  if (*fname=='.') {
    *ptr1++ = *fname++;
    if (_FP_stricmp (fname, "tar.gz") == 0) {
      *ptr1++ = 't';
      *ptr1++ = 'g';
      *ptr1++ = 'z';
    }
    else {
      if ((ptr2 = _FP_strrchr (fname, '.')) == NULL)
	ptr2 = fname;
      else
	ptr2++;
      count=0;
      while (*ptr2 && count++ < 3) {
	if (*ptr2 == ' ')
	  *ptr1++ = '_';
	else
	  *ptr1++ = *ptr2;
	ptr2++;
      }
    }
  }
  *ptr1 = '\0';
  return dosname;
}
