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
 * A minimal wish, extended by the UU commands
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

#ifdef HAVE_TK
#include <tk.h>
#else
#ifdef HAVE_TCL
#include <tcl.h>
#endif
#endif

char * uuwish_id = "$Id: uuwish.c,v 1.5 2001/06/21 14:05:47 fp Exp $";

/*
 * Tcl Init function
 */

int
Tcl_AppInit (Tcl_Interp *interp)
{
  extern int Uu_Init (Tcl_Interp *);

  if (Tcl_Init (interp) == TCL_ERROR) {
    return TCL_ERROR;
  }
#ifdef HAVE_TK
  if (Tk_Init (interp) == TCL_ERROR) {
    return TCL_ERROR;
  }
#endif

  if (Uu_Init (interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  /*
   * if (getenv("HOME")) {
   *   sprintf (tempstring, "%s/.xdeviewrc", getenv ("HOME"));
   *   Tcl_EvalFile (interp, tempstring);
   * }
   */

  return TCL_OK;
}

/*
 * uudeview/tcl main function
 */

int
main (int argc, char *argv[])
{
#ifdef HAVE_TK
  Tk_Main (argc, argv, Tcl_AppInit);
#else
#ifdef HAVE_TCL
  Tcl_Main (argc, argv, Tcl_AppInit);
#endif
#endif

  return 0;
}

