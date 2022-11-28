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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef SYSTEM_WINDLL
#include <windows.h>
#endif
#ifdef SYSTEM_OS2
#include <os2.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <uudeview.h>
#include <fptools.h>
#include <uufnflt.h>

/* For the sake of getcwd() in DOS */
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

/*
 * the progress meter is only displayed if stderr is a terminal.
 * take care of systems where we can't check this condition.
 */

#ifdef HAVE_ISATTY
#define UUISATTY(f)	(isatty(fileno(f)))
#else
#define UUISATTY(f)	(1)
#endif

#define SHOW_INFO	(1)	/* For more () */
#define SHOW_FILE	(2)

/*
 * local definitions
 */

typedef struct _extlist {
  char extension[8];
  struct _extlist *NEXT;
} extlist;

/*
 * Operational Flags
 */

static int incext    = 0, stdinput = 0;
static int overwrite = 0, interact = 1;
static int quiet     = 0, decoall  = 0;
static int nobar     = 0, autoren  = 0;

/*
 * Global Variables
 */

static extlist *extensions = NULL;
static char savepath[512];

/*
 * some statistics
 */

static int decodedok     = 0;
static int decodedfailed = 0;
static int decodedmisp   = 0;
static int inputerrors   = 0;
static int inputfiles    = 0;

/*
 * version string
 */

static char uuversion[256] = VERSION "pl" PATCH;

/*
 * cvs version
 */
char * uudeview_id = "$Id: uudeview.c,v 1.28 2003/04/12 23:33:55 fp Exp $";

static int	addexts		_ANSI_ARGS_((char *));
static int	work_file	_ANSI_ARGS_((char *));
static int	proc_stdin	_ANSI_ARGS_((void));
static void	loadparfile	_ANSI_ARGS_((char *));
static int	work_comline	_ANSI_ARGS_((int, char *[]));
static int	moreCB		_ANSI_ARGS_((void *, char *));
static void	more		_ANSI_ARGS_((uulist *, int));
static void	DumpFileList	_ANSI_ARGS_((void));
static int	process_one	_ANSI_ARGS_((uulist *));
static int	process_files	_ANSI_ARGS_((void));
static void	sighandler	_ANSI_ARGS_((int));
static void	usage		_ANSI_ARGS_((char *));

/*
 * Header lines starting with these texts will be stripped when
 * printing a file's info
 */

static char *hateheaders[] = {
  "Path:", "Newsgroups:", /*"Organization:",*/ "Lines:",
  "Message-ID:", "NNTP-Posting-Host:", "Xref:",
  "References:", "X-Newsreader:", "Distribution",
  /*"Sender:",*/ "Nntp-Posting-Host:", /*"Reply-To:",*/
  /*"Approved:",*/ "Mime-Version:", "Content-Type:",
  "Content-Transfer-Encoding:", "X-Posting-Software:",
  NULL
};

/*
 * Busy Callback
 */

static int
BusyCallback (void *param, uuprogress *progress)
{
  char stuff[26];
  int count, pcts;
  char *ptr;

  /*
   * Display progress meter only if stderr is a terminal
   */

  if (!UUISATTY(stderr) || nobar)
    return 0;

  if ((ptr = _FP_strrchr (progress->curfile, DIRSEPARATOR[0])) == NULL)
    ptr = progress->curfile;
  else
    ptr++;

  if (!quiet && progress->action == 1) {
    fprintf (stderr, "scanned %3d%% of %-50s\r",
	     progress->percent, ptr);
    fflush  (stderr);
  }
  else if (!quiet && progress->action == 2) {
    pcts = (int)((100*progress->partno+progress->percent-100) /
		 progress->numparts);
    for (count=0, stuff[25]='\0'; count<25; count++)
      stuff[count] = (count<pcts/4)?'#':'.';
    fprintf (stderr, "decoding part%3d of%3d %s\r",
	     progress->partno, progress->numparts,
	     stuff);
    fflush  (stderr);
  }
  else if (!quiet && progress->action == 3) {
    for (count=0, stuff[25]='\0'; count<25; count++)
      stuff[count] = (count<progress->percent/4)?'#':'.';
    fprintf (stderr, "copying target file      %s\r",
	     stuff);
    fflush  (stderr);
  }
  return 0;
}

/*
 * Message Callback
 */

static void
MessageCallback (void *param, char *message, int level)
{
  if (!quiet || level >= UUMSG_WARNING)
    fprintf (stderr, "%s\n", message);
}

/*
 * local functions
 */

static void
killext (extlist *data)
{
  extlist *iter=data, *ptr;

  while (iter) {
    ptr = iter->NEXT;
    _FP_free (iter);
    iter = ptr;
  }
}

static char *
filemode (int mode)
{
  static char result[11];
  static char rwxkey[8][4] = 
    { "---", "--x", "-w-", "-wx", 
      "r--", "r-x", "rw-", "rwx" };

  result[0] = '-';
  result[1] = rwxkey[((mode >> 6) & 0x07)][0];
  result[2] = rwxkey[((mode >> 6) & 0x07)][1];
  result[3] = rwxkey[((mode >> 6) & 0x07)][2];
  result[4] = rwxkey[((mode >> 3) & 0x07)][0];
  result[5] = rwxkey[((mode >> 3) & 0x07)][1];
  result[6] = rwxkey[((mode >> 3) & 0x07)][2];
  result[7] = rwxkey[  mode       & 0x07 ][0];
  result[8] = rwxkey[  mode       & 0x07 ][1];
  result[9] = rwxkey[  mode       & 0x07 ][2];
  result[10]= '\0';

  return result;
}

/*
 * check if we want the header displayed
 */

static int
IsHeaderLine (char *text)
{
  char **hiter = hateheaders;

  if (text == NULL || hiter == NULL)
    return 0;

  while (*hiter) {
    if (_FP_strnicmp (text, *hiter, strlen (*hiter)) == 0)
      return 1;
    hiter++;
  }
  return 0;
}

/*
 * get a character from stdin (the keyboard), or accept default
 */

static char 
getyn (char *theline, char def)
{
  char line[256], *ptr=0;
  int isdebug;

  if (interact && !feof (stdin)) {
    fflush (stdout);
    if ((ptr = _FP_fgets ((theline)?theline:line, 255, stdin)) == NULL) {
      if (theline) *theline='\0';
      return def;
    }
    if (ptr[0]=='4'&&ptr[1]=='2') {
      UUGetOption (UUOPT_DEBUG, &isdebug, NULL, 0);
      UUSetOption (UUOPT_DEBUG, !isdebug, NULL);
      if (!isdebug)
	printf ("*** Debugging Mode On\n");
      else
	printf ("*** Debugging Mode Off\n");
      ptr+=2;
    }
    while (isspace (*ptr) && *ptr!='\n')
      ptr++;
    if (*ptr == '\n') {
      if (theline) *theline='\0';
      return def;
    }
  }
  else {
    if (theline) *theline='\0';
    printf ("%c\n", def);
    return def;
  }
  return tolower(*ptr);
}

/*
 * check whether a file exists or not. This check is used when we decide
 * if a file would be overwritten. We also overwrite files with zero
 * length.
 */

static int
exists (char *filename)
{
  struct stat finfo;

  if (filename == NULL || *filename == '\0')
    return 0;

  if (stat (filename, &finfo) != 0)
    return 0;

  if ((long) finfo.st_size == 0)
    return 0;

  return 1;
}

/*
 * Find a new unique filename by adding a unique number to the file name.
 * Some want the unique name at the end, while others prefer to keep it
 * before the file extension. Support both.
 * So if autoren is positive, append the unique number at the end. If it
 * is negative, insert it before the first dot in the filename.
 */

int
findAlternateName (char *originalName, char *newName, int autoren)
{
  char *p;
  int i;

  for (i=1; i<32768; i++) {
    if (autoren > 0) {
      sprintf (newName, "%s.%d", originalName, i);
    }
    else {
      strcpy (newName, originalName);
      if ((p = strchr (newName, '.')) != NULL) {
	sprintf (p, "_%d%s", i, strchr (originalName, '.'));
      }
      else {
	sprintf (newName, "%s_%d", originalName, i);
      }
    }
    if (!exists (newName)) {
      return 1;
    }
  }

  return 0;
}

/*
 * Add another extension we shall or shall not decode
 */

static int 
addexts (char *elist)
{
  extlist *enew;
  int iter;

  while (*elist) {
    if (*elist != '.') {
      elist++;
      continue;
    }
    if (*++elist == '\0')
      break;

    if ((enew = (extlist *) malloc (sizeof (extlist))) == NULL) {
      fprintf (stderr, "ERROR: Out of memory in addexts()\n");
      return 0;
    }
    iter = 0;

    while (*elist != '.' && *elist != '\0' && iter < 7)
      enew->extension[iter++] = *elist++;

    enew->extension[iter] = '\0';

    enew->NEXT = extensions;
    extensions = enew;
  }
  return 1;
}

/*
 * looks at the extension of a file and decides whether we want to decode
 * it or not, based on our extensions list
 */

static int 
work_file (char *filename)
{
  extlist *iter = extensions;
  char *ptr;

  if (filename == NULL)
    return 0;

  if ((ptr = _FP_strrchr (filename, '.')) == NULL)
    return incext ? 0 : 1;

  ptr++;

  while (iter) {
    if (_FP_stricmp (ptr, iter->extension) == 0)
      return incext ? 1 : 0;

    iter = iter->NEXT;
  }

  return incext ? 0 : 1;
}

/*
 * Handle standard input. Dump it into a temporary file and then add
 * this temp file to our list of command-line files
 */

static int 
proc_stdin (void)
{
  static char buffer[1024];
  char *stdfile;
  FILE *target;
  size_t bytes;
  int res;

  if (stdinput) {
    fprintf (stderr, "proc_stdin: cannot process stdin twice\n");
    return 0;
  }

  if ((stdfile = tempnam (NULL, "uu")) == NULL) {
    fprintf (stderr, "proc_stdin: cannot get temporary file\n");
    return 0;
  }

  if ((target = fopen (stdfile, "wb")) == NULL) {
    fprintf (stderr, "proc_stdin: cannot open temp file %s for writing: %s\n",
	     stdfile, strerror (errno));
    _FP_free (stdfile);
    return 0;
  }

  while (!feof (stdin)) {
    bytes = fread (buffer, 1, 1024, stdin);

    if (bytes == 0)
      break;

    if (ferror (stdin)) {
      fprintf (stderr, "proc_stdin: error reading from stdin: %s\n",
	       strerror (errno));
      break;
    }

    if (fwrite (buffer, 1, bytes, target) != bytes) {
      fprintf (stderr, "proc_stdin: cannot write to temp file %s: %s\n",
	       stdfile, strerror (errno));
      break;
    }

    if (ferror (target)) {
      fprintf (stderr, "proc_stdin: error writing to temp file %s: %s\n",
	       stdfile, strerror (errno));
      break;
    }
  }

  if (ferror (stdin) || ferror (target)) {
    fclose (target);
    unlink (stdfile);
    _FP_free (stdfile);
    return 0;
  }
  fclose (target);

  inputfiles++;
  if ((res = UULoadFile (stdfile, NULL, 1)) != UURET_OK) {
    fprintf (stderr, "ERROR: while reading from copied standard input %s: %s %s\n",
	     stdfile, UUstrerror (res),
	     (res==UURET_IOERR)?
	     strerror (UUGetOption (UUOPT_ERRNO, NULL, NULL, 0)) : "");
    inputerrors = 1;
  }
  _FP_free (stdfile);
  stdinput = 1;

  return 1;
}

/*
 * extract parameters from "command line"
 */

static void
makeparams (char *comline)
{
  char *p1, *argv[32];
  int argc=2, quote=0;

  if (comline==NULL || *comline=='\0')
    return;

  p1 = comline;

  while (*p1 == ' ' || *p1 == '\t') {
    p1++;
  }

  argv[1] = p1;

  while (*p1 && argc < 31) {
    switch (*p1) {
    case ' ':
    case '\t':
      if (!quote) {
	if ((*argv[argc-1] == '"' || *argv[argc-1] == '\'') &&
	    p1-1 != argv[argc-1] &&
	    *(p1-1) == *argv[argc-1]) {
	  *(p1-1) = '\0';
	  argv[argc-1] += 1;
	}
	*p1++ = '\0';
	while (*p1 == ' ' || *p1 == '\t')
	  p1++;
	argv[argc++] = p1;
      }
      else {
	p1++;
      }
      break;

    case '"':
    case '\'':
      if (!quote) {
	quote = *p1++;
      }
      else if (quote == *p1++) {
	quote = 0;
      }
      break;
      
    default:
      p1++;
    }
  }

  if (!quote) {
    if ((*argv[argc-1] == '"' || *argv[argc-1] == '\'') &&
	p1-1 != argv[argc-1] &&
	*(p1-1) == *argv[argc-1]) {
      *(p1-1) = '\0';
      argv[argc-1] += 1;
    }
  }

  work_comline (argc, argv);
}

/*
 * Load a file with command-line parameters (given with @)
 */

static void 
loadparfile (char *filename)
{
  char line[256], c;
  FILE *pfile;

  if ((pfile = fopen (filename+1, "r")) == NULL) {
    fprintf (stderr, "Couldn't load parameter file %s: %s (ignored)\n",
	     filename+1, strerror (errno));
    return;
  }

  while (!feof (pfile)) {
    if (_FP_fgets (line, 256, pfile) == NULL)
      break;

    if (ferror (pfile))
      break;

    line[strlen(line)-1] = '\0';

    if (strlen (line) == 0)
      continue;

    if (line[strlen(line)-1] == 0x0a || line[strlen(line)-1] == 0x0d)
        line[strlen(line)-1] = '\0';

    if (line[0] == '\0' || line[0] == '#')
        continue;

    makeparams (line);

    c = fgetc (pfile);
    if (feof (pfile))
      break;
    else
      ungetc (c, pfile);
  }

  fclose (pfile);

  /*
   * command line files are always removed
   */

  unlink (filename+1);
}

/*
 * process command line parameters
 */

static int 
work_comline (int argc, char *argv[])
{
  int number, res;

  for (number=1; number<argc; number++) {
    if (*argv[number] == '@')
      loadparfile (argv[number]);
    else if (*argv[number] != '-' && *argv[number] != '+') {
      inputfiles++;
      if ((res = UULoadFile ((*argv[number]=='|')?argv[number]+1:argv[number],
			     NULL,
			     (*argv[number]=='|')?1:0)) != UURET_OK) {
	fprintf (stderr, "ERROR: while reading from %s: %s %s\n",
		 (*argv[number]=='|')?argv[number]+1:argv[number],
		 UUstrerror (res),
		 (res==UURET_IOERR)?
		 strerror (UUGetOption (UUOPT_ERRNO, NULL, NULL, 0)) : "");
	inputerrors++;
      }
    }
    else switch (*(argv[number] + 1)) {
    case '\0':
      interact = 0;
      if (overwrite == 0) {
	overwrite = 1;
      }
      proc_stdin ();
      break;
    case 'a':
      autoren = (*argv[number] == '+') ? -1 : 1;
      break;
    case 'b':
      if (argv[number][2] == '0' || argv[number][2] == '(')
	UUSetOption (UUOPT_BRACKPOL, 0, NULL);
      else if (argv[number][2] == '1' || argv[number][2] == '[')
	UUSetOption (UUOPT_BRACKPOL, 1, NULL);
      else if (argv[number][2] == '\0')
	UUSetOption (UUOPT_BRACKPOL, 1, NULL);
      /* '])' Emacs feels happier if I close these brackets */
      break;
    case 'c':
      UUSetOption (UUOPT_REMOVE, 1, NULL);
      break;
    case 'd':
      UUSetOption (UUOPT_DESPERATE, 1, NULL);
      UUSetOption (UUOPT_TINYB64,   1, NULL);
      break;
    case 'e':
      if (number+1 < argc && *argv[number+1] == '.') {
        incext = (*argv[number] == '+') ? 1 : 0;
        addexts (argv[++number]);
      }
      break;
    case 'f':
      UUSetOption (UUOPT_FAST, 1, NULL);
      break;
    case 'h':
      usage (argv[0]);
      exit (99);
      break;
    case 'i':
      if (*argv[number] == '+' && stdinput) {
	fprintf (stderr, "WARNING: cannot interact when reading from stdin\n");
      }
      else {
	interact  = (*argv[number] == '+') ? 1 : 0;
	if (overwrite == 0 && *argv[number] == '-') {
	  overwrite = 1;
	}
      }
      break;
    case 'm':
      UUSetOption (UUOPT_IGNMODE, 1, NULL);
      break;
    case 'n':
      nobar = 1;
      break;
    case 'o':
      if (*argv[number] == '-') {
	overwrite = 1;
      }
      else {
	overwrite = -1;
      }
      break;
    case 'p':
      if (number+1 < argc) {
        strcpy (savepath, argv[++number]);
	if (strlen (savepath)) {
	  if (savepath[strlen(savepath)-1]!=DIRSEPARATOR[0])
	    strcat (savepath, DIRSEPARATOR);
	}
      }
      break;
    case 'q':
      UUSetOption (UUOPT_VERBOSE, 0, NULL);
      quiet = 1;
      nobar = 1;
      break;
    case 'r':
      UUSetOption (UUOPT_IGNREPLY, 1, NULL);
      break;
    case 's':
      if (argv[number][2] == '\0')
	UUSetOption (UUOPT_DUMBNESS,
		     UUGetOption (UUOPT_DUMBNESS, NULL, NULL, 0) + 1,
		     NULL);
      else
	UUSetOption (UUOPT_DUMBNESS, 42, NULL);
      break;
    case 't':
      UUSetOption (UUOPT_USETEXT, 1, NULL);
      break;
    case 'v':
      UUSetOption (UUOPT_VERBOSE, 0, NULL);
      break;
    case 'V':
      fprintf (stdout, "uudeview %spl%s compiled on %s\n",
	       VERSION, PATCH, __DATE__);
      exit (0);
      break;
    case 'z':
      if (argv[number][2] >= '0' && argv[number][2] <= '9') {
	UUSetOption (UUOPT_MOREMIME, argv[number][2]-'0', NULL);
      }
      else if (argv[number][2] == 'z') {
	UUSetOption (UUOPT_MOREMIME, 2, NULL);
      }
      else {
	res = UUGetOption (UUOPT_MOREMIME, NULL, NULL, 0);
	UUSetOption (UUOPT_MOREMIME, res+1, NULL);
      }
      break;
    default:
      usage (argv[0]);
      exit (99);
      break;
    }
  }

  return 1;
}

/*
 * list a file callback
 */

struct mCBparm {
  int aline, lines;
  int quitit, cols;
};

static int
moreCB (void *param, char *string)
{
  struct mCBparm *data = (struct mCBparm *) param;

  if (IsHeaderLine (string))
    return 0;

  if (data->aline+2 >= data->lines) {
    data->aline = 0;
    if (interact) {
      fprintf (stdout, "<<MORE -- (q) to quit>>");
      if (getyn (NULL, 'y') == 'q') {
	data->quitit = 1;
	return 1;
      }
    }
  }

  while (strlen(string) &&
	 (string[strlen(string)-1] == '\012' ||
	  string[strlen(string)-1] == '\015'))
    string[strlen(string)-1] = '\0';

  if (data->cols > 0)
    string[data->cols-1] = '\0';

  fprintf (stdout, "%s\n", string);
  data->aline += 1;

  return 0;
}

static void
more (uulist *uin, int type)
{
  struct mCBparm data;
  FILE *inpfile = NULL;
  char text[256], *p;
  int res;

  data.aline  = 0;
  data.quitit = 0;

  if ((p = getenv ("LINES")) != NULL) {
    if ((data.lines = atoi(p)) < 5)
      data.lines = 24;
  }
  else
    data.lines = 24;

  if ((p = getenv ("COLUMNS")) != NULL) {
    if ((data.cols = atoi(p)) < 30)
      data.cols = 80;
  }
  else
    data.cols = 80;

  if (uin == NULL || uin->thisfile == NULL) {
    printf ("\tError -- (?)\n");
    return;
  }
  if (type == SHOW_INFO) {
    UUInfoFile (uin, &data, moreCB);
  }
  else {
    if ((res = UUDecodeToTemp (uin)) != UURET_OK) {
      fprintf (stderr, "ERROR: while decoding %s (%s): %s\n",
	       uin->filename, (uin->subfname) ? uin->subfname : "",
	       UUstrerror(res));
      return;
    }
    if (UUISATTY(stderr) && !nobar) {
      fprintf (stderr, "%70s\r", ""); /* clear progress information */
      fflush  (stderr);
    }

    if ((inpfile = fopen (uin->binfile, "r")) == NULL) {
      fprintf (stderr, "ERROR: could not open %s: %s\n",
	       uin->binfile, strerror (errno));
      return;
    }

    while (!feof (inpfile)) {
      if (_FP_fgets (text, data.cols, inpfile) == NULL)
	break;

      if (ferror (inpfile)) {
	fprintf (stderr, "ERROR: while reading from %s: %s\n",
		 uin->binfile, strerror (errno));
	break;
      }

      if (moreCB (&data, text))
	break;
    }
    fclose (inpfile);
  }

  if (interact && data.aline+2 >= data.lines) {
    fprintf (stdout, "<<END -- hit return>>");
    getyn   (NULL, '?');
  }
  else
    fprintf (stdout, "<<END>>");
}

static void
DumpFileList (void)
{
  int count, printed=0, index=0;
  uulist *iter;

  while ((iter=UUGetFileListItem(index))) {
    if (iter->state & UUFILE_NODATA) {
      index++;
      continue;
    }

    if (UUISATTY(stderr) && !nobar) {
      fprintf (stderr, "\r%70s\r", "");
      fflush  (stderr);
    }

    if (!printed++)
      printf ("\n");

    printf ("Found '%s' State %d %s Parts ",
	    (iter->filename) ? iter->filename :
	    (iter->subfname) ? iter->subfname : "???",
	    iter->state,
	    (iter->uudet == UU_ENCODED) ? "UUdata" :
	    (iter->uudet == B64ENCODED) ? "Base64" :
	    (iter->uudet == XX_ENCODED) ? "XXdata" :
	    (iter->uudet == BH_ENCODED) ? "Binhex" : 
	    (iter->uudet == YENC_ENCODED) ? "yEnc" : "Text");

    if (iter->haveparts) {
      printf ("%s%d%s ",
	      (iter->begin && iter->begin==iter->haveparts[0]) ? "begin " : "",
	      iter->haveparts[0],
	      (iter->end   && iter->end == iter->haveparts[0]) ? " end" : "");
      for (count=1; iter->haveparts[count]; count++) {
	printf ("%s%d%s ",
		(iter->begin==iter->haveparts[count]) ? "begin " : "",
		iter->haveparts[count],
		(iter->end == iter->haveparts[count]) ? " end"   : "");
      }
    }
    if (iter->state & UUFILE_OK) {
      printf ("OK");
    }
    printf ("\n");
    index++;
  }
  if (printed)
    printf ("\n");
}

/*
 * Returning  1 skips to next file
 *           -1 skips to prev file
 *            0 quits program
 */

static int 
process_one (uulist *iter)
{
  char targetname[1024], renamedname[1024];
  char line[1024], command[256], tmp, *ptr1, *ptr2;
  int res, escflag;
  char *name;

  while (42) {
    if ((name = UUFNameFilter (iter->filename)) == NULL) {
      fprintf (stderr, "ERROR: couldn't get filename of %s (%s)\n",
	       (iter->filename)?iter->filename:"(null)",
	       (iter->subfname)?iter->subfname:"(null)");
      break;
    }

    if (interact && !decoall) {
      printf ("  %s %-15s is %s   [d] (?=help) ",
	      filemode((int)iter->mode),
	      (iter->filename) ? (iter->filename) : "",
	      (iter->state&UUFILE_OK) ? "OK" : "in error (DESPERATE MODE)");
      tmp = getyn (line, 'd');
    }
    else {
      line[0] = '\0';
      tmp = 'd';
    }

    for (ptr1=line; *ptr1 && *ptr1 != tmp; ptr1++);
    if (*ptr1++==tmp) {
      while (isspace(*ptr1)) ptr1++;
    }

    if (tmp == 'n')
      return 1;
    if (tmp == 'b') {
      if (iter->PREV == NULL)
	printf ("*** Already at the beginning of the list\n");
      else
	return -1;
      continue;
    }
    else if (tmp == 'a') {
      decoall = 1;
      continue;
    }
    else if (tmp == 'c') {
      printf ("\n\n-------------------------------------------------------------------------------\n\n");
      printf ("\tYou are now using UUDEVIEW, the uudecoder by Frank Pilhofer\n\n");
      printf ("\tThe program  is  distributed  under  the  terms  of the GNU\n");
      printf ("\tGeneral Public License.  You should have received a copy of\n");
      printf ("\tthe GPL with the uudeview program. In particular this means\n");
      printf ("\tthat the program is distributed without any warranty.\n\n");
      printf ("\tIf you like uudeview, you are encouraged to send the author\n");
      printf ("\ta postcard (find  the address  in the  README  file), or at\n");
      printf ("\tleast an email.\n\n");
      printf ("\tCheck out uudeview's home page:\n");
      printf ("\t\thttp://www.fpx.de/fp/Software/UUDeview/\n\n");
      printf ("\t            Frank Pilhofer (fp@fpx.de)\n\n");
      printf ("-------------------------------------------------------------------------------\n\n");
      continue;
    }
    else if (tmp == 'q') {
      return 0;
    }
    else if (tmp == '?') {
      printf ("\n\tYou can ...\n");
      printf ("\t  - (d)ecode the file\n");
      printf ("\t  - (y)es - same as (d)\n");
      printf ("\t  - e(x)tract - same as (d)\n");
      printf ("\t  - decode (a)ll files\n");
      printf ("\t  - go to (n)ext file\n");
      printf ("\t  - show file (i)nfo\n");
      /*
       * Can't execute anything in QuickWin
       */
#ifndef SYSTEM_QUICKWIN
      printf ("\t  - (e)xecute a command\n");
#endif      
      printf ("\t  - (l)ist textfile\n");
      printf ("\t  - (r)ename file\n");
      printf ("\t  - decode (a)ll files\n");
      printf ("\t  - go (b)ack to the previous file\n");
      printf ("\t  - set decode (p)ath: %s\n", savepath);
      printf ("\t  - (q)uit program\n");
      printf ("\t  - display (c)opyright\n\n");
      continue;
    }
    else if (tmp == 'r') {
      if (strlen(ptr1) <= 1) {
	printf ("Enter new filename: "); fflush (stdout);
	if (_FP_fgets (line, 250, stdin) == NULL) {
	  printf ("\nERROR: Could not get filename: %s\n",
		  strerror (errno));
	  line[0] = '\0';
	}
	ptr1 = line;
      }
      if (strlen (ptr1) > 1) {
	if (ptr1[strlen(ptr1)-1] == '\n')
	  ptr1[strlen(ptr1)-1] = '\0';
	UURenameFile (iter, ptr1);
      }
      continue;
    }
    else if (tmp == 'p') {
      if (strlen(ptr1) <= 1) {
	printf ("Enter new path: "); fflush (stdout);
	if (_FP_fgets (line, 250, stdin) == NULL) {
	  printf ("\nERROR: Could not get path: %s\n", strerror (errno));
	  line[0] = '\0';
	}
	ptr1 = line;
      }
      if (strlen (ptr1) > 1) {
	if (ptr1[strlen(ptr1)-1] == '\n')
	  ptr1[strlen(ptr1)-1] = '\0';
	/* check that path exists */
	if (access(ptr1, W_OK|X_OK) == 0) {
	  struct stat finfo;
	  if (stat (ptr1, &finfo) != 0) {
	    printf ("\nERROR: Could not get path: %s\n",
		    strerror (errno));
	    break;
	  }
	  if (!S_ISDIR(finfo.st_mode)) {
	    printf("\nERROR: `%s' is not a directory\n", ptr1);
	    break;
	  }
	}
	strcpy (savepath, ptr1);
	if (strlen (savepath)) {
	  if (savepath[strlen(savepath)-1]!=DIRSEPARATOR[0])
	    strcat (savepath, DIRSEPARATOR);
	}
      }
      continue;
    }
    else if (tmp == 'i') {
      printf ("\nFile info ...\n\n");
      printf ("-------------------------------------------------------------------------------\n");
      more   (iter, SHOW_INFO);
      printf ("\n-------------------------------------------------------------------------------\n\n");
      continue;
    }
    /*
     * for the following menu items, we need the file decoded
     */
    if ((res = UUDecodeToTemp (iter)) != UURET_OK) {
      fprintf (stderr, "ERROR: while decoding %s (%s): %s\n",
	       (iter->filename) ? iter->filename : "",
	       (iter->subfname) ? iter->subfname : "",
	       UUstrerror(res));
      decodedfailed++;
      break;
    }
    if (!quiet && UUISATTY(stderr) && !nobar) {
      fprintf (stderr, "%70s\r", ""); /* clear progress information */
      fflush  (stderr);
    }
    if (iter->binfile == NULL) {
      fprintf (stderr, "ERROR: Ooops. Decoded %s but no binfile??\n",
	       (iter->filename) ? iter->filename : "");
      decodedfailed++;
      break;
    }
    
    if (tmp=='d' || tmp=='y' || tmp=='x' || tmp=='\0' || tmp=='\n') {
      /*
       * Set Target directory
       */
    make_target:
      sprintf (targetname, "%s%s%s",
	       savepath, 
	       (strlen(savepath) &&
		savepath[strlen(savepath)-1]==DIRSEPARATOR[0]) ?
	       "":DIRSEPARATOR,
	       name);

      /*
       * check whether this file exists
       */

    retryexcheck:
      if (exists (targetname) && overwrite!=1 && autoren &&
	  findAlternateName (targetname, renamedname, autoren)) {
	printf ("*** Target File %s exists - autorenamed to %s\n",
		targetname, renamedname);
	strcpy (targetname, renamedname);
      }

      if (exists (targetname) && overwrite == -1) {
	printf ("*** Target File %s exists! (file not decoded)\n",
		targetname);
	return 1;
      }

      if (exists (targetname) && !overwrite) {
	printf ("*** Target File %s exists! Options:\n", targetname);
	res = ' ';

	printf ("\
*** (O)verwrite, Overwrite (A)ll, Overwrite Non(e),\n\
*** (R)ename, Au(t)o-Rename, (N)ext, (P)ath [o] ");
	fflush (stdout);

	/*
	 * Ask the user for her/his choice. If 'r' or 'p', and there
	 * is more data on the line, use it as reply
	 */

	res = getyn (line, stdinput ? 'n' : 'o');

	for (ptr1=line; *ptr1 && tolower(*ptr1) != res; ptr1++);
	if (*ptr1++==res) {
	  while (isspace(*ptr1)) ptr1++;
	}

	switch (res) {
	case 'n': return 1;
	case 'a': overwrite=1; goto retryexcheck;
	case 'o': goto noexcheck;
	case 'e': overwrite=-1; return 1;
	case 't': autoren=1; goto retryexcheck;
	case 'r': 
	  if (strlen(ptr1) <= 1) {
	    printf ("Enter new filename: "); fflush (stdout);
	    if (_FP_fgets (line, 250, stdin) == NULL) {
	      printf ("\nERROR: Could not get filename: %s\n",
		      strerror (errno));
	      line[0] = '\0';
	    }
	    ptr1 = line;
	  }
	  if (strlen (ptr1) > 1) {
	    if (ptr1[strlen(ptr1)-1] == '\n')
	      ptr1[strlen(ptr1)-1] = '\0';
	    UURenameFile (iter, ptr1);
	  }
	  if ((name = UUFNameFilter (iter->filename)) == NULL) {
	    fprintf (stderr, "ERROR: couldn't get filename of %s (%s)\n",
		     (iter->filename)?iter->filename:"(null)",
		     (iter->subfname)?iter->subfname:"(null)");
	    return 1;
	  }
	  goto make_target;

	case 'p':
	  if (strlen(ptr1) <= 1) {
	    printf ("Enter new path: "); fflush (stdout);
	    if (_FP_fgets (line, 250, stdin) == NULL) {
	      printf ("\nERROR: Could not get path: %s\n",
		      strerror (errno));
	      line[0] = '\0';
	    }
	    ptr1 = line;
	  }
	  if (strlen (ptr1) > 1) {
	    if (ptr1[strlen(ptr1)-1] == '\n')
	      ptr1[strlen(ptr1)-1] = '\0';
	    strcpy (savepath, ptr1);
	    if (strlen(savepath)) {
	      if (savepath[strlen(savepath)-1]!=DIRSEPARATOR[0])
		strcat (savepath, DIRSEPARATOR);
	    }
	  }
	  goto make_target;

	default:
	  goto retryexcheck;
	}
      }

    noexcheck:
      if ((res = UUDecodeFile (iter, targetname)) != UURET_OK) {
	if (UUISATTY(stderr) && !nobar)
	  fprintf (stderr, "%70s\r", ""); /* clear progress information */
	fprintf (stderr, "ERROR: while writing %s (%s): %s\n",
		 targetname, (iter->subfname) ? iter->subfname : "",
		 UUstrerror(res));
	decodedfailed++;
	break;
      }
      if (!quiet) {
	if (UUISATTY(stderr) && !nobar)
	  fprintf (stderr, "%70s\r", ""); /* clear progress information */
	printf  ("    File successfully written to %s\n", targetname);
      }
      decodedok++;
      break;
    }
    else if (tmp == 'l') {
      printf ("\nContents of file ...\n\n");
      printf ("------------------------------------------------------------------------------\n");
      more   (iter, SHOW_FILE);
      printf ("\n------------------------------------------------------------------------------\n\n");
    }
    /*
     * Can't do that in QuickWin, since we'd need system()
     */
#ifndef SYSTEM_QUICKWIN
    else if (tmp == 'e') {
      printf ("Enter command line ($) for file: "); fflush (stdout);
      if (_FP_fgets (line, 256, stdin) == NULL)
	printf ("\nERROR: Could not get Command line: %s\n",
		strerror (errno));
      else if (strlen (line) > 1) {
	ptr1 = line; ptr2 = command; escflag = 0;

	while (*ptr1 && *ptr1 != '\012' && *ptr1 != '\015') {
	  if (!escflag && *ptr1 != '\\') {
	    if (*ptr1 == '$') {
	      strcpy (ptr2, iter->binfile);
	      ptr2  += strlen (iter->binfile);
	      ptr1++;
	    }
	    else
	      *ptr2++ = *ptr1++;
	  }
	  else if (escflag) {
	    *ptr2++ = *ptr1++;
	    escflag = 0;
	  }
	  else
	    escflag = 1;
	}
	*ptr2 = '\0';

	printf ("------------------------------------------------------------------------------\n");
	system (command);
	printf ("------------------------------------------------------------------------------\n\n");
      }
    }
#endif	
    else {
      printf ("ERROR: unknown action '%c'. Enter ? for list of options.\n", 
	      tmp);
    }
  }

  return 1;
}
    
static int 
process_files (void)
{
  int res, index=0, desp;
  uulist *iter;
  char *ptr;

  while ((iter=UUGetFileListItem(index))) {
    if (iter->filename == NULL) {
      index++;
      continue;
    }
    if (!(work_file (iter->filename))) {
      if (interact && !quiet)
        printf ("  %s %s ignored.\n",
                filemode((int)iter->mode), 
		(iter->filename)?iter->filename:"");
      
      index++;
      continue;
    }
    if (iter->state & UUFILE_OK)
      ptr = "State is OK";
    else if (iter->state & UUFILE_NODATA)
      ptr = NULL;
    else if (iter->state & UUFILE_NOBEGIN)
      ptr = "No Begin found";
    else if (iter->state & UUFILE_MISPART)
      ptr = "Missing Part(s)";
    else if (iter->state & UUFILE_NOEND)
      ptr = "No End found";
    else
      ptr = "Unknown State";

    if (iter->state & UUFILE_NODATA) {
      index++;
      continue;
    }

    UUGetOption (UUOPT_DESPERATE, &desp, NULL, 0);

    if (iter->state & UUFILE_OK || desp) {
      res = process_one (iter);
    }
    else {
      if (ptr && iter->filename)
        printf ("ERROR: File %s (%s): %s (%d)\n", 
                (iter->filename) ? iter->filename : "",
		(iter->subfname) ? iter->subfname : "",
		ptr, iter->state);
      decodedmisp++;
      res  = 1;
    }

    if (res == 0)
      break;
    else if (res == -1) {
      if (index==0)
	printf ("*** Already at beginning of list\n");
      else {
	index--;
	while ((!(iter->state & UUFILE_OK || desp) ||
		iter->state == UUFILE_NODATA) && index)
	  index--;
      }
    }
    else {
      index++;
    }
  }
  return 0;
}

static void
sighandler (int signo)
{
  printf ("\nReceived Signal (%d), cleaning up temp files.\n", signo);

  UUCleanUp ();
  exit (99);
}

/*
 * usage
 */

static void 
usage (char *argv0)
{
  printf ("\n\
  UUDEVIEW %s%s%s - the nice and friendly decoder - (w) 1994 Frank Pilhofer\n",
	  VERSION, (PATCH[0]>'0')?"pl":"", (PATCH[0]>'0')?PATCH:"");
  printf ("  usage:\n");
  printf ("    uudeview [options] [file ...]\n\n");
  printf ("  Options:\n");
  printf ("\t-i      Disable interactivity (do not ask, decode everything)\n");
  printf ("\t-a      Autorename (rename file if it exists on disk)\n");
  printf ("\t-m      Ignore the file mode of uuencoded files\n");
  printf ("\t+e/-e   Include or exclude extensions exclusively\n");
  printf ("\t-d      Sets 'desperate' mode (process incomplete files)\n");
  printf ("\t-f      Fast mode. Only if each file holds no more than one part\n");
  printf ("\t-o/+o   -o OK to overwrite files, +o never overwrite files\n");
  printf ("\t-b1     Select alternate bracket policy\n");
  printf ("\t-p path Sets path where to save decoded binaries to\n");
  printf ("\t-c      Autoclear files that were successfully decoded.\n");
  printf ("\t-s      Be more strict on MIME adherance.\n");
  printf ("\t-q      Quiet. Do not emit progress messages.\n\n");
#if defined(SYSTEM_DOS) || defined(SYSTEM_QUICKWIN)
  printf ("  See Manual for more details\n\n");
#else
  printf ("  See uudeview(1) for more details.\n\n");
#endif
  printf ("  Example:\n");
  printf ("    uudeview +e .jpg.gif -i newsfile\n");
  printf ("\tThis decodes all .jpg or .gif files encountered in <newsfile>\n");
  printf ("\twithout asking.\n\n");
}

/*
 * uudeview main function
 */

int
main (int argc, char *argv[])
{
  int res;
#ifdef SYSTEM_QUICKWIN
  struct _wsizeinfo ws;
#endif

  /*
   * No Signal handler in QuickWin
   */
#ifndef SYSTEM_QUICKWIN
  signal (SIGINT, sighandler);
#endif
  /*
   * In QuickWin, set the about() Box, and give more space to scroll
   */
#ifdef SYSTEM_QUICKWIN
  ws._version = _QWINVER;
  ws._type    = _WINSIZEMAX;
  (void) _wabout        ("UUdeview for Windows\n(c) 1995 Frank Pilhofer\nfp@fpx.de");
  (void) _wsetscreenbuf (fileno(stdout), 16384);
  (void) _wsetscreenbuf (fileno(stderr), 16384);
  (void) _wsetsize      (fileno(stdout), &ws);
#endif  

  /*
   * Check where we are and set the save directory
   */
#ifdef HAVE_GETCWD
  if (getcwd (savepath, 255) == NULL)
#endif
    strcpy (savepath, "./");

  /*
   * in DOS, set the DOS Filename Filter
   */

#if defined(SYSTEM_DOS) || defined(SYSTEM_QUICKWIN)
  UUSetFNameFilter (NULL, UUFNameFilterDOS);
#else
  UUSetFNameFilter (NULL, UUFNameFilterUnix);
#endif

  UUSetBusyCallback (NULL, BusyCallback, 100);

  /*
   * If we were called as uudecode, be quiet and don't interact
   */
  if (_FP_stristr (argv[0], "uudecode") != NULL) {
    interact    = 0;
    decoall     = 1;
    quiet       = 1;
    overwrite   = 1;
  }
  else if (argc < 2) {
    usage (argv[0]);
    return 99;
  }

  /*
   * Setup Callback
   */

  UUSetMsgCallback (NULL, MessageCallback);

  if (UUInitialize () != UURET_OK) {
    fprintf (stderr, "oops: could not initialize decoding library\n");
    return 99;
  }

  /*
   * use options from UUDEVIEW environment variable
   */

  if ((getenv ("UUDEVIEW")) != NULL) {
    makeparams (getenv ("UUDEVIEW"));
  }

  if (argc < 2) {
    /*
     * can only be in uudecode compatibility mode
     */
    proc_stdin ();
  }
  else {
    work_comline (argc, argv);
  }

  if (strlen(savepath)) {
    if (savepath[strlen(savepath)-1] != DIRSEPARATOR[0])
      strcat (savepath, DIRSEPARATOR);
  }

  if (!stdinput && !quiet && UUGetOption (UUOPT_VERBOSE, NULL, NULL, 0)) {
    DumpFileList ();
  }

  /*
   * try merging thrice with increased tolerance
   */
  UUSmerge (0);
  UUSmerge (1);
  UUSmerge (99);

  res = process_files ();

  /*
   * clear info
   */
  if (UUISATTY(stderr) && !nobar) {
    fprintf (stderr, "\r%70s\r", "");
    fflush  (stderr);
  }

#ifndef SYSTEM_QUICKWIN
  signal   (SIGINT, SIG_DFL);
#endif
  UUCleanUp();
  killext  (extensions);

  /*
   * Without user interaction, or if the user has quit
   * the proggy, kill myself in QuickWin
   */
#ifdef SYSTEM_QUICKWIN
  if (!interact || res==1)
    _wsetexit (_WINEXITNOPERSIST);
  else {
    printf ("\n\
No more Programs to decode.\n\
Select File-Exit to close window\n\
\n");
  }
#endif  

  if (UUGetOption (UUOPT_VERBOSE, NULL, NULL, 0) && !quiet &&
      inputfiles && (decodedok || decodedfailed || decodedmisp)) {
    printf ("%d file%s decoded from %d input file%s, %d failed\n",
	    decodedok, (decodedok==1)?"":"s",
	    inputfiles, (inputfiles==1)?"":"s",
	    decodedfailed+decodedmisp);
  }

  /*
   * determine return value
   */

  if (decodedfailed || decodedmisp) {
    return ((decodedok?1:0) | ((decodedmisp||decodedfailed)?2:0));
  }

  return 0;
}

