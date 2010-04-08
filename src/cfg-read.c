#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfg-read.h"
#include "cfg-parse.h"
#include "list.h"

static CopaStatus
cfg_read_block_start (const char * name, void * user_data)
{
  struct cfg * cfg = (struct cfg *) user_data;

  if (cfg->block_dict != NULL) {
	  fprintf (stderr, "Illegal nested configuration block\n");
	  return COPA_STOP_ERR;
  }

  cfg->block_dict = dictionary_new();

  return COPA_OK;
}

static CopaStatus
cfg_read_block_end (const char * name, void * user_data)
{
  struct cfg * cfg = (struct cfg *) user_data;

  if (cfg->block_dict == NULL) {
	  fprintf (stderr, "Block end outside block\n");
	  return COPA_STOP_ERR;
  }

  dictionary_delete (cfg->block_dict);
  cfg->block_dict = NULL;

  return COPA_OK;
}

static CopaStatus
cfg_read_assign (const char * name, const char * value, void * user_data)
{
  struct cfg * cfg = (struct cfg *) user_data;
  Dictionary * dict;

  if (cfg->block_dict != NULL)
	  dict = cfg->block_dict;
  else
	  dict = cfg->dictionary;

  dictionary_insert (dict, name, value);

  return COPA_OK;
}

struct cfg *
cfg_read (const char * path)
{
  CopaStatus status;
  struct cfg * cfg;

  cfg = calloc (1, sizeof(*cfg));

  cfg->dictionary = dictionary_new ();
  cfg->block_dict = NULL;

  cfg->resources = list_new ();

  status = copa_read (path, cfg_read_block_start, cfg,
		      cfg_read_block_end, cfg,
		      cfg_read_assign, cfg);

  if (status != COPA_OK) {
    free (cfg);
    return NULL;
  }

  return cfg;
}

