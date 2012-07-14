#include <stdio.h>

extern int load_dll (char *name);

int
main (int argc, char **argv)
{
  printf ("try to load DLL %s\n", argv[1]);

  load_dll (argv[1]);
}
