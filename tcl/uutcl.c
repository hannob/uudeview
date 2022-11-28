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
 * The TCL Interface of UUDeview
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_TCL) || defined(HAVE_TK)

#ifdef SYSTEM_WINDLL
#include <windows.h>
#endif
#ifdef SYSTEM_OS2
#include <os2.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_TK
#include <tk.h>
#else
#include <tcl.h>
#endif

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

#include <uudeview.h>
#include <uuint.h>
#include <fptools.h>

/*
 * As Windows DLL, we need a DllEntryPoint
 */

#ifdef SYSTEM_WINDLL
BOOL _export WINAPI 
DllEntryPoint (HINSTANCE hInstance, DWORD seginfo,
	       LPVOID lpCmdLine)
{
  /* Don't do anything, so just return true */
  return TRUE;
}
#endif

/*
 * Declare external functions as __cdecl for Watcom C
 */

#ifdef __WATCOMC__
#pragma aux (__cdecl) Tcl_Eval
#pragma aux (__cdecl) Tcl_GetVar
#pragma aux (__cdecl) Tcl_SetVar
#pragma aux (__cdecl) Tcl_AppendResult
#pragma aux (__cdecl) Tcl_SetResult
#pragma aux (__cdecl) Tcl_CreateCommand
#endif

/*
 * cvs version
 */
char * uutcl_id = "$Id: uutcl.c,v 1.14 2002/03/06 13:52:45 fp Exp $";

/*
 * data for our Callbacks
 */

static struct uutclcbdata {
  Tcl_Interp *interp;
  char tclproc[256];
} theDMcbdata, theBusycbdata;

/*
 * Don't let Uu_Init initialize us twice
 */

static int uu_AlreadyInitialized = 0;

/*
 * mail and news software
 */

#ifdef PROG_INEWS
char *  uue_inewsprog = PROG_INEWS;
#else
char *  uue_inewsprog = NULL;
#endif
#ifdef PROG_MAILER
char *  uue_mailprog  = PROG_MAILER;
#else
char *  uue_mailprog  = NULL;
#endif
#ifdef MAILER_NEEDS_SUBJECT
int     uue_mpsubject = 1;
#else
int     uue_mpsubject = 0;
#endif

/*
 * Mail or Post a file. Remember to keep in sync with uuenview.c
 */

static int
SendAFile (Tcl_Interp *interp,
	   FILE *infile,   char *infname,
	   int encoding,   int linperfile,
	   char *outfname, char *towhom,
	   char *subject,  char *from,
	   char *replyto,  int isemail)
{
  char *command, *rcptlist, *ptr;
  FILE *thepipe, *theifile;
  int len, count, res, part;

  if (towhom==NULL ||
      (outfname==NULL&&infname==NULL) || (infile&&infname==NULL) ||
      (encoding!=UU_ENCODED&&encoding!=XX_ENCODED&&encoding!=B64ENCODED&&
       encoding!=PT_ENCODED&&encoding!=QP_ENCODED)) {
    Tcl_SetResult (interp, "oops: Parameter check failed in SendAFile()",
		   TCL_STATIC);
    return UURET_ILLVAL;
  }

#ifndef HAVE_POPEN
  Tcl_SetResult (interp, "error: Your system does not support sending of files",
		 TCL_STATIC);
  return UURET_ILLVAL;
#else
  if (isemail && (uue_mailprog == NULL || *uue_mailprog == '\0')) {
    Tcl_SetResult (interp, "error: Cannot Email file: option not configured",
		   TCL_STATIC);
    return UURET_ILLVAL;
  }
  else if (!isemail && (uue_inewsprog == NULL || *uue_inewsprog == '\0')) {
    Tcl_SetResult (interp, "error: Cannot Post file: option not configured",
		   TCL_STATIC);
    return UURET_ILLVAL;
  }

  len = strlen ((isemail)?uue_mailprog:uue_inewsprog) + 
    ((uue_mpsubject)?strlen(subject):0) +
      ((isemail)?0:strlen(towhom)) + 32;

  if ((command = (char *) malloc (len)) == NULL) {
    Tcl_SetResult (interp, "error: Out of memory allocating some bytes",
		   TCL_STATIC);
    return UURET_NOMEM;
  }

  if ((rcptlist = (char *) malloc (strlen (towhom) + 16)) == NULL) {
    Tcl_SetResult (interp, "error: Out of memory allocating some bytes",
		   TCL_STATIC);
    _FP_free (command);
    return UURET_NOMEM;
  }

  if (isemail) {
    if (uue_mpsubject)
      sprintf (command, "%s -s \"%s\"", uue_mailprog, subject);
    else
      sprintf (command, "%s", uue_mailprog);

    /*
     * Attach list of recipients to mailer command and compose another list
     * of recipients
     */

    count = 0;
    rcptlist[0] = '\0';
    ptr = _FP_strtok (towhom, ",; ");

    while (ptr) {
      strcat (command, " ");
      strcat (command, ptr);

      if (count++)
	strcat (rcptlist, ",");
      strcat (rcptlist, ptr);

      ptr = _FP_strtok (NULL, ",; ");
    }
  }
  else {
    sprintf (command, "%s", uue_inewsprog);

    count = 0;
    rcptlist[0] = '\0';
    ptr = _FP_strtok (towhom, ";, ");

    while (ptr) {
      if (count++)
	strcat (rcptlist, ",");
      strcat (rcptlist, ptr);
      ptr = _FP_strtok (NULL, ";, ");
    }
  }

  if (from && *from == '\0') {
    from = NULL;
  }

  if (subject && *subject == '\0') {
    subject = NULL;
  }

  if (replyto && *replyto == '\0') {
    replyto = NULL;
  }
  
  /*
   * Get going ...
   */

  if (infile == NULL) {
    if ((theifile = fopen (infname, "rb")) == NULL) {
      _FP_free (rcptlist);
      _FP_free (command);
      return UURET_IOERR;
    }
  }
  else {
    theifile = infile;
  }

  for (part=1; !feof (theifile); part++) {
    if ((thepipe = popen (command, "w")) == NULL) {
      if (infile==NULL) fclose (theifile);
      _FP_free (rcptlist);
      _FP_free (command);
      return UURET_IOERR;
    }

    if (UUGetOption(UUOPT_VERBOSE, NULL, NULL, 0)) {
#if 0
      fprintf (stderr, "%s part %03d of %s to %s ... ",
	       (isemail)?"mailing":"posting",
	       part, (infname)?infname:outfname,
	       rcptlist);
      fflush  (stderr);
#endif
    }

    res = UUE_PrepPartialExt (thepipe, theifile, infname, encoding,
			      outfname, 0, part, linperfile, 0,
			      rcptlist, from, subject, replyto,
			      isemail);

#if 0
    if (UUGetOption (UUOPT_VERBOSE, NULL, NULL, 0)) {
      if (res == UURET_OK)
	fprintf (stderr, "ok.\n");
      else
	fprintf (stderr, "%s\n", UUstrerror (res));
    }
#endif

    pclose (thepipe);

    if (res != UURET_OK) {
      if (infile == NULL) fclose (theifile);
      _FP_free (rcptlist);
      _FP_free (command);
      return res;
    }
  }

  if (infile == NULL) fclose (theifile);
  _FP_free (rcptlist);
  _FP_free (command);
  return UURET_OK;
#endif
}

/*
 * Display a Message in a dialog box
 */

static void
uutcl_DisplayMessage (void *param, char *message, int level)
{
  struct uutclcbdata *data = (struct uutclcbdata *) param;
  char tmpstring[2048];

  if (data->interp && *data->tclproc) {
    sprintf  (tmpstring, "%s %d {%s}\n", data->tclproc, level, message);
    Tcl_Eval (data->interp, tmpstring);
  }
}

/*
 * Our Busy Callback
 */

static int
uutcl_BusyCallback (void *param, uuprogress *progress)
{
  struct uutclcbdata *data = (struct uutclcbdata *) param;

  if (data->interp && *data->tclproc) {
    Tcl_Eval (data->interp, data->tclproc);
  }
  return 0;
}

/*
 * exchage variables with the interpreter
 */

static void
uutcl_UpdateParameter (Tcl_Interp *interp)
{
  char *cval;

  if ((cval = Tcl_GetVar (interp, "OptionFast",     TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_FAST, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionBracket",  TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_BRACKPOL, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionDesperate",TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_DESPERATE, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionDebug",    TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_DEBUG, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionDumbness", TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_DUMBNESS, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionUsetext",  TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_USETEXT, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "SaveFilePath",   TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_SAVEPATH, 0, cval);
  if ((cval = Tcl_GetVar (interp, "OptionRemove",   TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_REMOVE, atoi (cval), NULL);
  if ((cval = Tcl_GetVar (interp, "OptionMoreMime", TCL_GLOBAL_ONLY))!=NULL)
    UUSetOption (UUOPT_MOREMIME, atoi (cval), NULL);
}

/*
 * configuration info
 */

static int
uutcl_HaveArg (int argc, char *argv[], char *string)
{
  int index;

  for (index=1; index<argc; index++) {
    if (_FP_stricmp (argv[index], string) == 0)
      return 1;
  }
  return 0;
}

static int UUTCLFUNC
uutcl_Info (ClientData clientData, Tcl_Interp *interp,
	    int argc, char *argv[])
{
  char temp[64], version[64];

  if (argc==1 || uutcl_HaveArg (argc, argv, "version")) {
    UUGetOption (UUOPT_VERSION, NULL, version, 64);
    sprintf (temp, " { version %s } ", version);
    Tcl_AppendResult (interp, temp, NULL);
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "have_tcl"))
#ifdef HAVE_TCL
    Tcl_AppendResult (interp, " { have_tcl 1 } ", NULL);
#else
    Tcl_AppendResult (interp, " { have_tcl 0 } ", NULL);
#endif
  if (argc==1 || uutcl_HaveArg (argc, argv, "have_tk"))
#ifdef HAVE_TK
    Tcl_AppendResult (interp, " { have_tk 1 } ", NULL);
#else
    Tcl_AppendResult (interp, " { have_tk 0 } ", NULL);
#endif
  if (argc==1 || uutcl_HaveArg (argc, argv, "have_mail")) {
#ifdef PROG_MAILER
    if (*PROG_MAILER)
      Tcl_AppendResult (interp, " { have_mail 1 } ", NULL);
    else
      Tcl_AppendResult (interp, " { have_mail 0 } ", NULL);
#else
    Tcl_AppendResult (interp, " { have_mail 0 } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "prog_mailer")) {
#ifdef PROG_MAILER
    sprintf (temp, " { prog_mailer \"%s\" } ", PROG_MAILER);
    Tcl_AppendResult (interp, temp, NULL);
#else
    Tcl_AppendResult (interp, " { prog_mailer (none) } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "have_news")) {
#ifdef HAVE_NEWS
    Tcl_AppendResult (interp, " { have_news 1 } ", NULL);
#else
    Tcl_AppendResult (interp, " { have_news 0 } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "prog_inews")) {
#ifdef PROG_INEWS
    sprintf (temp, " { prog_inews \"%s\" } ", PROG_INEWS);
    Tcl_AppendResult (interp, temp, NULL);
#else
    Tcl_AppendResult (interp, " { prog_inews (none) } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "domainname")) {
#ifdef DOMAINNAME
    sprintf (temp, " { domainname %s } ", DOMAINNAME);
    Tcl_AppendResult (interp, temp, NULL);
#else
    Tcl_AppendResult (interp, " { domainname (none) } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "nntpserver")) {
#ifdef NNTPSERVER
    sprintf (temp, " { nntpserver %s } ", NNTPSERVER);
    Tcl_AppendResult (interp, temp, NULL);
#else
    Tcl_AppendResult (interp, " { nntpserver (none) } ", NULL);
#endif
  }
  if (argc==1 || uutcl_HaveArg (argc, argv, "need_nntpserver")) {
#ifdef NEED_NNTPSERVER
    Tcl_AppendResult (interp, " { need_nntpserver 1 } ", NULL);
#else
    Tcl_AppendResult (interp, " { need_nntpserver 0 } ", NULL);
#endif
  }
  return TCL_OK;
}

static int UUTCLFUNC
uutcl_SetMessageProc (ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  char tmpstring[256];

  uutcl_UpdateParameter (interp);

  if (argc != 2) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s procedure\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  theDMcbdata.interp = interp;
  strcpy (theDMcbdata.tclproc, argv[1]);
  return TCL_OK;
}

static int UUTCLFUNC
uutcl_SetBusyProc (ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  char tmpstring[256];
  long msecs;

  uutcl_UpdateParameter (interp);

  if (argc != 3) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s procedure msecs\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((msecs = (long) atoi (argv[2])) > 0) {
    UUSetBusyCallback (&theBusycbdata, uutcl_BusyCallback, msecs);
  }

  theBusycbdata.interp = interp;
  strcpy (theBusycbdata.tclproc, argv[1]);
  return TCL_OK;
}

static int UUTCLFUNC
uutcl_GetProgressInfo (ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
  uuprogress progress;
  char tmpstring[32];

  if (UUGetOption (UUOPT_PROGRESS, NULL, 
		   (char *) &progress, sizeof (uuprogress)) != 0) {
    Tcl_SetResult (interp, "oops, could not get info?", TCL_STATIC);
    return TCL_ERROR;
  }
  sprintf (tmpstring, "%d", progress.action);
  Tcl_AppendElement (interp, tmpstring);

  Tcl_AppendElement (interp, progress.curfile);

  sprintf (tmpstring, "%d", progress.partno);
  Tcl_AppendElement (interp, tmpstring);

  sprintf (tmpstring, "%d", progress.numparts);
  Tcl_AppendElement (interp, tmpstring);

  sprintf (tmpstring, "%d", progress.percent);
  Tcl_AppendElement (interp, tmpstring);

  return TCL_OK;
}

static int UUTCLFUNC
uutcl_GetListOfFiles (ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
  char tmpstring[1024], t2[42];
  int count=0, index=0;
  uulist *iter;

  uutcl_UpdateParameter (interp);

  while ((iter=UUGetFileListItem(count))) {
    if (((iter->state & UUFILE_OK) || 
	 UUGetOption (UUOPT_DESPERATE, NULL, NULL, 0)) && iter->filename) {
      sprintf (tmpstring, " { %d %d {%s} %s %s {",
	       count, iter->state, iter->filename,
	       (iter->mimetype)?iter->mimetype:"{}",
	       (iter->uudet == UU_ENCODED) ? "UUdata " :
	       (iter->uudet == B64ENCODED) ? "Base64 " :
	       (iter->uudet == XX_ENCODED) ? "XXdata " :
	       (iter->uudet == BH_ENCODED) ? "Binhex " : 
	       (iter->uudet == YENC_ENCODED) ? "yEnc" : "Text");

      if (iter->haveparts) {
	sprintf (t2, "%s%s%d ",
		 (iter->begin&&iter->begin==iter->haveparts[0])?"begin ":"",
		 (iter->end  &&iter->end == iter->haveparts[0])?"end "  :"",
		 iter->haveparts[0]);
	strcat (tmpstring, t2);

	for (index=1; iter->haveparts[index]; index++) {
	  sprintf (t2, "%s%s%d ",
		   (iter->begin==iter->haveparts[index]) ? "begin " : "",
		   (iter->end == iter->haveparts[index]) ? "end " : "",
		   iter->haveparts[index]);
	  strcat (tmpstring, t2);
	}
      }

      if (iter->state & UUFILE_OK)
	strcat (tmpstring, "OK");

      strcat (tmpstring, "} }");

      Tcl_AppendResult (interp, tmpstring, NULL);
    }
    count++;
  }

  return TCL_OK;
}

/*
 * Load an encoded file
 */

static int UUTCLFUNC
uutcl_LoadFile (ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  char tmpstring[256];
  int res;

  uutcl_UpdateParameter (interp);

  if (argc != 2) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s filename\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((res = UULoadFile (argv[1], NULL, 0)) != UURET_OK) {
    sprintf (tmpstring, "couldn't read %s: %s (%s)",
	     argv[1], UUstrerror (res),
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 * Decode A File. This function overwrites files without asking, because
 * this was already done by the script
 */

static int UUTCLFUNC
uutcl_DecodeFile (ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
  char tmpstring[256];
  uulist *iter;
  int res;

  uutcl_UpdateParameter (interp);

  if (argc < 2 || argc > 3) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s number ?targetname?\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((iter = UUGetFileListItem (atoi (argv[1]))) == NULL) {
    Tcl_SetResult (interp, "invalid file number", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((res = UUDecodeFile (iter, (argc==3)?argv[2]:NULL)) != UURET_OK) {
    sprintf (tmpstring, "Error while decoding %s (%s): %s (%s)",
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     UUstrerror (res),
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  return TCL_OK;
}

static int UUTCLFUNC
uutcl_GetTempFile (ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
  char tmpstring[256];
  uulist *iter;
  int res;

  uutcl_UpdateParameter (interp);

  if (argc != 2) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s number\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((iter = UUGetFileListItem (atoi (argv[1]))) == NULL) {
    Tcl_SetResult (interp, "invalid file number", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((res = UUDecodeToTemp (iter)) != UURET_OK) {
    sprintf (tmpstring, "Error while decoding %s (%s): %s (%s)",
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     UUstrerror (res), 
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (iter->binfile == NULL) {
    Tcl_SetResult (interp, "unknown error while decoding", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_SetResult (interp, iter->binfile, TCL_VOLATILE);
  return TCL_OK;
}

/*
 * InfoFile takes two parameters: the number of the file to get info for,
 * and the name of the text widget to send the text to
 */

struct uuInfoCBData {
  Tcl_Interp *interp;
  char *      widget;
};

static int
uutcl_InfoCallback (void *param, char *string)
{
  struct uuInfoCBData *data = (struct uuInfoCBData *) param;
  char tmpstring[1024], *p;

  sprintf (tmpstring, "%s insert end \"", data->widget);

  p = tmpstring + strlen (tmpstring);

  while (*string) {
    switch (*string) {
    case '"':
    case '\\':
    case '[':
    case ']':
    case '$':
      *p++ = '\\';
      /* fallthrough */
    default:
      *p++ = *string;
    }
    string++;
  }
  *p++ = '"';
  *p++ = '\0';

  if (Tcl_Eval (data->interp, tmpstring) != TCL_OK)
    return 1;

  return 0;
}

static int UUTCLFUNC
uutcl_InfoFile (ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  struct uuInfoCBData data;
  char tmpstring[256];
  uulist *iter;
  int res;

  uutcl_UpdateParameter (interp);

  if (argc != 3) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s number textwidget\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((iter = UUGetFileListItem (atoi (argv[1]))) == NULL) {
    Tcl_SetResult (interp, "invalid file number", TCL_STATIC);
    return TCL_ERROR;
  }

  sprintf  (tmpstring, "%s delete 1.0 end", argv[2]);
  if (Tcl_Eval (interp, tmpstring) != TCL_OK)
    return TCL_ERROR;

  data.interp = interp;
  data.widget = argv[2];

  if ((res = UUInfoFile (iter, &data, uutcl_InfoCallback)) != UURET_OK) {
    sprintf (tmpstring, "Error while getting info for %s (%s): %s (%s)",
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     UUstrerror (res), 
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 * ShowFile takes two parameters: the number of the file to get info for,
 * and the name of the text widget to send the text to. We might have to
 * decode the file before we can show it.
 * Hey, the above callback worked so well, let's use it again!
 */

static int UUTCLFUNC
uutcl_ListFile (ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
  uulist *iter;
  struct uuInfoCBData data;
  char tmpstring[1024];
  FILE *inpfile;
  int res;

  uutcl_UpdateParameter (interp);

  if (argc != 3) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s number textwidget\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ((iter = UUGetFileListItem (atoi (argv[1]))) == NULL) {
    Tcl_SetResult (interp, "invalid file number", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((res = UUDecodeToTemp (iter)) != UURET_OK) {
    sprintf (tmpstring, "Error while decoding %s (%s): %s (%s)",
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     UUstrerror(res), 
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }
    
  sprintf  (tmpstring, "%s delete 1.0 end", argv[2]);
  if (Tcl_Eval (interp, tmpstring) != TCL_OK)
    return TCL_ERROR;

  if (iter->binfile==NULL || (inpfile=fopen (iter->binfile, "r"))==NULL) {
    Tcl_SetResult (interp, "couldn't read file", TCL_STATIC);
    return TCL_ERROR;
  }
  if ((inpfile = fopen (iter->binfile, "r")) == NULL) {
    sprintf (tmpstring, "Could not open temp file %s of %s (%s): %s",
	     iter->binfile, 
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     strerror (errno));
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  data.interp = interp;
  data.widget = argv[2];

  while (!feof (inpfile)) {
    if (_FP_fgets (tmpstring, 512, inpfile) == NULL)
      break;

    if (ferror (inpfile))
      break;

    if (uutcl_InfoCallback (&data, tmpstring))
      break;
  }

  if (ferror (inpfile)) {
    sprintf (tmpstring, "Error while reading from temp file %s of %s (%s): %s",
	     iter->binfile, 
	     (iter->filename) ? iter->filename : "",
	     (iter->subfname) ? iter->subfname : "",
	     strerror (errno));
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    fclose (inpfile);
    return TCL_ERROR;
  }

  fclose (inpfile);
  return TCL_OK;
}

static int UUTCLFUNC
uutcl_Rename (ClientData clientData, Tcl_Interp *interp,
	      int argc, char *argv[])
{
  char tmpstring[256];
  uulist *iter;
  int res;

  uutcl_UpdateParameter (interp);

  if (argc != 3) {
    sprintf (tmpstring,
	     "wrong # args: should be \"%s number newname\"",
	     argv[0]);
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (*argv[2] == '\0') {
    Tcl_SetResult (interp, "illegal file name", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((iter = UUGetFileListItem (atoi (argv[1]))) == NULL) {
    Tcl_SetResult (interp, "invalid file number", TCL_STATIC);
    return TCL_ERROR;
  }

  if ((res = UURenameFile (iter, argv[2])) != UURET_OK) {
    sprintf (tmpstring,
	     "could not rename %s to %s: %s (%s)",
	     (iter->filename) ? iter->filename : "",
	     argv[2], UUstrerror (res),
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, tmpstring, TCL_VOLATILE);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 * clean up memory and temp files
 */

static int UUTCLFUNC
uutcl_CleanUp (ClientData clientData, Tcl_Interp *interp,
	       int argc, char *argv[])
{
  uutcl_UpdateParameter (interp);

  UUCleanUp ();

  return TCL_OK;
}

/*
 * Generic function to extract the encoding and linperfile parameters
 * from the command's command line
 */

static int
uutcl_GetEncodeParams (Tcl_Interp *interp,
		       int argc,  char *argv[],
		       int argv1, int *encoding,
		       int argv2, int *linperfile)
{
  if (argv2 && argv2 < argc) {
    *linperfile = atoi (argv[argv2]);

    if (*linperfile != 0 && *linperfile < 200) {
      Tcl_SetResult (interp, "illegal number of lines per file", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  if (argv1 && argv1 < argc) {
    switch (*argv[argv1]) {
    case '0':
    case 'u':
    case 'U':
      *encoding = UU_ENCODED;
      break;
    case '1':
    case 'x':
    case 'X':
      *encoding = XX_ENCODED;
      break;
    case '2':
    case 'b':
    case 'B':
      *encoding = B64ENCODED;
      break;
    case '3':
    case 't':
    case 'T':
      *encoding = PT_ENCODED;
      break;
    case '4':
    case 'q':
    case 'Q':
      *encoding = QP_ENCODED;
      break;
    case '5':
    case 'y':
    case 'Y':
      *encoding = YENC_ENCODED;
      break;
    default:
      Tcl_SetResult (interp, "invalid encoding method", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

/*
 * Encode and store in a file.
 * Syntax:
 *    uu_EncodeToFile source path \
 *                    [ dest subject intro lines encoding from replyto]
 *
 * Most arguments are just for compatibilty with the other encoding procs.
 */

static int UUTCLFUNC
uutcl_EncodeToFile (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  int encoding=UU_ENCODED, linperfile=0, res;
  char errstring[256], olddir[256];

  if (argc < 3 || argc > 10) {
    Tcl_SetResult (interp, "wrong # args", TCL_STATIC);
    return TCL_ERROR;
  }

  uutcl_UpdateParameter (interp);

  if (uutcl_GetEncodeParams (interp, argc, argv, 
			     7, &encoding, 
			     6, &linperfile) != TCL_OK)
    return TCL_ERROR;

  UUGetOption (UUOPT_SAVEPATH, NULL, olddir, 256);
  UUSetOption (UUOPT_SAVEPATH, 0, argv[2]);

  if ((res = UUEncodeToFile (NULL, argv[1], encoding,
			     (argc>3) ? argv[3] : NULL,
			     (argc>2) ? argv[2] : NULL,
			     linperfile)) != UURET_OK) {
    UUSetOption (UUOPT_SAVEPATH, 0, olddir);
    sprintf (errstring, "error while encoding %s to file: %s (%s)", argv[1],
	     UUstrerror(res), 
	     (res==UURET_IOERR)?
	     strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
    Tcl_SetResult (interp, errstring, TCL_VOLATILE);
    return TCL_ERROR;
  }
  UUSetOption (UUOPT_SAVEPATH, 0, olddir);
  return TCL_OK;
}

/*
 * Encode and send by email
 * Syntax:
 *    uu_EncodeToMail source addr \
 *                    [ dest subject intro lines encoding from replyto ]
 *
 * addr can be a single address or a list of addresses
 */

static int UUTCLFUNC
uutcl_EncodeToMail (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  int encoding=UU_ENCODED, linperfile=0, res;
  char errstring[256];

  if (argc < 3 || argc > 10) {
    Tcl_SetResult (interp, "wrong # args", TCL_STATIC);
    return TCL_ERROR;
  }
  uutcl_UpdateParameter (interp);

  if (uutcl_GetEncodeParams (interp, argc, argv, 
			     7, &encoding, 
			     6, &linperfile) != TCL_OK)
    return TCL_ERROR;

  if ((res = SendAFile (interp, NULL, argv[1], encoding, linperfile,
			/* outfname */ (argc>3) ? argv[3] : NULL,
			/* towhom   */ argv[2],
			/* subject  */ (argc>4) ? argv[4] : NULL,
			/* from     */ (argc>8) ? argv[8] : NULL,
			/* replyto  */ (argc>9) ? argv[9] : NULL,
			1)) != UURET_OK) {
    /*
     * If res==UURET_ILLVAL, SendAMail has already filled in the result
     */
    if (res != UURET_ILLVAL) {
      sprintf (errstring, "error while emailing %s: %s (%s)", argv[1],
	       UUstrerror(res), 
	       (res==UURET_IOERR)?
	       strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
      Tcl_SetResult (interp, errstring, TCL_VOLATILE);
    }
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 * Encode and post to the news
 * Syntax:
 *    uu_EncodeToNews source addr \
 *                    [ dest subject intro lines encoding from replyto ]
 *
 * addr can be a single newsgroup or a list of newsgroups
 */

static int UUTCLFUNC
uutcl_EncodeToNews (ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
  int encoding=UU_ENCODED, linperfile=0, res;
  char errstring[256];

  if (argc < 3 || argc > 10) {
    Tcl_SetResult (interp, "wrong # args", TCL_STATIC);
    return TCL_ERROR;
  }
  uutcl_UpdateParameter (interp);

  if (uutcl_GetEncodeParams (interp, argc, argv, 
			     7, &encoding, 
			     6, &linperfile) != TCL_OK)
    return TCL_ERROR;
  
  if ((res = SendAFile (interp, NULL, argv[1], encoding, linperfile,
			/* outfname */ (argc>3) ? argv[3] : NULL,
			/* towhom   */ argv[2],
			/* subject  */ (argc>4) ? argv[4] : NULL,
			/* from     */ (argc>8) ? argv[8] : NULL,
			/* replyto  */ (argc>9) ? argv[9] : NULL,
			0)) != UURET_OK) {
    /*
     * If res==UURET_ILLVAL, SendAMail has already filled in the result
     */
    if (res != UURET_ILLVAL) {
      sprintf (errstring, "error while posting %s: %s (%s)", argv[1],
	       UUstrerror(res), 
	       (res==UURET_IOERR)?
	       strerror(UUGetOption(UUOPT_ERRNO,NULL,NULL,0)):"");
      Tcl_SetResult (interp, errstring, TCL_VOLATILE);
    }
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 * Initialize the TCL package. The only function that is exported from
 * this module.
 */

int UUTCLEXPORT UUTCLFUNC
Uu_Init (Tcl_Interp *interp)
{
  char tmp[32];

  /*
   * Check whether we are already initialized
   */

  if (uu_AlreadyInitialized++)
    return TCL_OK;

  /*
   * Initialize decoding engine
   */

  if (UUInitialize () != UURET_OK) {
    Tcl_SetResult (interp, "Error initializing decoding engine", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * register commands
   */

  Tcl_CreateCommand (interp, "uu_Info",          uutcl_Info, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_SetMessageProc",uutcl_SetMessageProc,
		     NULL, NULL);
  Tcl_CreateCommand (interp, "uu_SetBusyProc",   uutcl_SetBusyProc,NULL,NULL);
  Tcl_CreateCommand (interp, "uu_GetProgressInfo",uutcl_GetProgressInfo,
		     NULL, NULL);
  Tcl_CreateCommand (interp, "uu_GetListOfFiles",uutcl_GetListOfFiles,
		     NULL, NULL);
  Tcl_CreateCommand (interp, "uu_LoadFile",      uutcl_LoadFile, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_DecodeFile",    uutcl_DecodeFile, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_GetTempFile",   uutcl_GetTempFile,NULL,NULL);
  Tcl_CreateCommand (interp, "uu_InfoFile",      uutcl_InfoFile, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_ListFile",      uutcl_ListFile, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_Rename",        uutcl_Rename, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_CleanUp",       uutcl_CleanUp, NULL, NULL);
  Tcl_CreateCommand (interp, "uu_EncodeToFile",  uutcl_EncodeToFile,NULL,NULL);
  Tcl_CreateCommand (interp, "uu_EncodeToMail",  uutcl_EncodeToMail,NULL,NULL);
  Tcl_CreateCommand (interp, "uu_EncodeToNews",  uutcl_EncodeToNews,NULL,NULL);

  /*
   * our message-handling function and busy callback
   */

  theDMcbdata.interp       = NULL;
  theDMcbdata.tclproc[0]   = '\0';
  UUSetMsgCallback (&theDMcbdata, uutcl_DisplayMessage);

  theBusycbdata.interp     = NULL;
  theBusycbdata.tclproc[0] = '\0';
  UUSetBusyCallback (&theBusycbdata, uutcl_BusyCallback, 1000);

  /*
   * only set variables if they aren't set already
   */

  sprintf (tmp, "%d", UUGetOption (UUOPT_FAST, NULL, NULL, 0));
  if (Tcl_GetVar (interp, "OptionFast", TCL_GLOBAL_ONLY) == NULL)
    Tcl_SetVar (interp, "OptionFast", tmp, TCL_GLOBAL_ONLY);

  sprintf (tmp, "%d", UUGetOption (UUOPT_BRACKPOL, NULL, NULL, 0));
  if (Tcl_GetVar (interp, "OptionBracket", TCL_GLOBAL_ONLY) == NULL)
    Tcl_SetVar (interp, "OptionBracket", tmp, TCL_GLOBAL_ONLY);

  sprintf (tmp, "%d", UUGetOption (UUOPT_DESPERATE, NULL, NULL, 0));
  if (Tcl_GetVar (interp, "OptionDesperate", TCL_GLOBAL_ONLY) == NULL)
    Tcl_SetVar (interp, "OptionDesperate", tmp, TCL_GLOBAL_ONLY);

  sprintf (tmp, "%d", UUGetOption (UUOPT_DEBUG, NULL, NULL, 0));
  if (Tcl_GetVar (interp, "OptionDebug", TCL_GLOBAL_ONLY) == NULL)
    Tcl_SetVar (interp, "OptionDebug", tmp, TCL_GLOBAL_ONLY);

  sprintf (tmp, "%d", UUGetOption (UUOPT_USETEXT, NULL, NULL, 0));
  if (Tcl_GetVar (interp, "OptionUsetext", TCL_GLOBAL_ONLY) == NULL)
    Tcl_SetVar (interp, "OptionUsetext", tmp, TCL_GLOBAL_ONLY);

  return TCL_OK;
}

#endif
