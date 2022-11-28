#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <config.h>
#include <uudeview.h>
#include <fptools.h>

void MsgCallBack (void *opaque, char *msg, int level)
{
  fprintf (stderr, "%s\n", msg);
}

char * FNameFilter (void *opaque, char *fname)
{
  static char dname[13];
  char *p1, *p2;
  int i;

  if ((p1 = _FP_strrchr (fname, '/')) == NULL)
    p1 = fname;
  if ((p2 = _FP_strrchr (p1, '\\')) == NULL)
    p2 = p1;
  for (i=0, p1=dname; *p2 && *p2!='.' && i<8; i++)
    *p1++ = (*p2==' ')?(p2++,'_'):*p2++;
  while (*p2 && *p2 != '.') p2++;
  if ((*p1++ = *p2++) == '.')
    for (i=0; *p2 && *p2!='.' && i<3; i++)
      *p1++ = (*p2==' ')?(p2++,'_'):*p2++;
  *p1 = '\0';
  return dname;
}

int main (int argc, char *argv[])
{
  uulist *item;
  int i, res;

  UUInitialize     ();
  UUSetMsgCallback (NULL, MsgCallBack);
  UUSetFNameFilter (NULL, FNameFilter);

  for (i=1; i<argc; i++)
    if ((res = UULoadFile (argv[i], NULL, 0)) != UURET_OK)
      fprintf (stderr, "could not load %s: %s\n",
	       argv[i], (res==UURET_IOERR) ?
	       strerror (UUGetOption (UUOPT_ERRNO, NULL, NULL, 0)) :
	       UUstrerror(res));

  for (i=0; (item=UUGetFileListItem(i)) != NULL; i++) {
    if ((item->state & UUFILE_OK) == 0)
      continue;
    if ((res = UUDecodeFile (item, NULL)) != UURET_OK) {
      fprintf (stderr, "error decoding %s: %s\n",
	       (item->filename==NULL)?"oops":item->filename,
	       (res==UURET_IOERR) ?
	       strerror (UUGetOption (UUOPT_ERRNO, NULL, NULL, 0)) :
	       UUstrerror(res));
    }
    else {
      printf ("successfully decoded '%s' as '%s'\n",
	      item->filename,
	      UUFNameFilter (item->filename));
    }
  }
  UUCleanUp ();
  return 0;
}
