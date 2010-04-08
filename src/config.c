#include <stdlib.h>
#include <string.h>

#include "dictionary.h"
#include "cfg-parse.h"

static CopaStatus
config_block_start (const char * name, void * user_data)
{
  return COPA_OK;
}

static CopaStatus
config_assign (const char * name, const char * value, void * user_data)
{
  Dictionary * dictionary = (Dictionary *) user_data;

  dictionary_insert (dictionary, name, value);

  return COPA_OK;
}

int
config_read (const char * path, Dictionary * dictionary)
{
  CopaStatus status;

  status = copa_read (path, config_block_start, dictionary,
		      config_assign, dictionary);

  return (status == COPA_OK) ? 0 : -1;
}

