#include <stdlib.h>
#include <strings.h>

#include "dictionary.h"

int
main (int argc, char * argv[])
{
  Dictionary * table;
  const char * value;

  table = dictionary_new ();
  dictionary_delete (table);

  table = dictionary_new ();
  dictionary_insert (table, "pants", "off");
  dictionary_insert (table, "dog", "pat");
  dictionary_insert (table, "fish", "heads");

  value = dictionary_lookup (table, "fish");
  if (!value) exit (1);
  if (strcmp (value, "heads")) exit (1);

  value = dictionary_lookup (table, "pants");
  if (!value) exit (1);
  if (strcmp (value, "off")) exit (1);

  value = dictionary_lookup (table, "dog");
  if (!value) exit (1);
  if (strcmp (value, "pat")) exit (1);

  dictionary_insert (table, "pants", "on");
  value = dictionary_lookup (table, "pants");
  if (!value) exit (1);
  if (strcmp (value, "on")) exit (1);

  dictionary_delete (table);

  exit (0);
}
