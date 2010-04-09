
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg-parse.h"

static int
dump_block (const char * name, void * user_data)
{
  if (!strcasecmp (name, "bollocks"))
    return COPA_SKIP_BLOCK;
  printf ("[%s]\n", name);
  return COPA_OK;
}

static int
dump_assign (const char * name, const char * value, void * user_data)
{
  printf ("%s = %s\n", name, value);
  return 0;
}

int
main (int argc, char * argv[])
{
  if (argc < 1) {
    printf ("Usage: %s file\n", argv[0]);
    exit (0);
  }

  copa_read (argv[1], dump_block, NULL, NULL, NULL, dump_assign, NULL);
  
  exit (0);
}
