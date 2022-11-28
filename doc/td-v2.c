#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <config.h>
#include <uudeview.h>

int main (int argc, char *argv[])
{
  uulist *item;
  int i, res;

  UUInitialize ();
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
      printf ("successfully decoded '%s'\n", item->filename);
    }
  }
  UUCleanUp ();
  return 0;
}
