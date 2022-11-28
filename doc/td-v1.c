#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <uudeview.h>

int main (int argc, char *argv[])
{
  UUInitialize ();
  UULoadFile   (argv[1], NULL, 0);
  UUDecodeFile (UUGetFileListItem (0), NULL);
  UUCleanUp    ();
  return 0;
}

