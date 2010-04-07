#include <stdlib.h>
#include <string.h>

#include "dictionary.h"
#include "cfg-parse.h"

static XiniStatus
config_section (const char * name, void * user_data)
{
  return XINI_OK;
}

static XiniStatus
config_assign (const char * name, const char * value, void * user_data)
{
  Dictionary * dictionary = (Dictionary *) user_data;

  dictionary_insert (dictionary, name, value);

  return XINI_OK;
}

int
config_read (const char * path, Dictionary * dictionary)
{
  XiniStatus status;

  status = xini_read (path, config_section, dictionary,
		      config_assign, dictionary);

  return (status == XINI_OK) ? 0 : -1;
}

