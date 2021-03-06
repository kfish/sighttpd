/*
   Copyright (C) 2009 Conrad Parker
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

  if (!strncasecmp (name, "StaticText", 10)) {
	  cfg->resources = list_join (cfg->resources, statictext_resources (cfg->block_dict));
  } else if (!strncasecmp (name, "Stdin", 5)) {
          cfg->resources = list_join (cfg->resources, fdstream_resources (cfg->block_dict));
#ifdef HAVE_OGGZ
  } else if (!strncasecmp (name, "OggStdin", 8)) {
          cfg->resources = list_join (cfg->resources, oggstdin_resources (cfg->block_dict));
#endif
#ifdef HAVE_SHCODECS
  } else if (!strncasecmp (name, "SHRecord", 8)) {
	  cfg->resources = list_join (cfg->resources, shrecord_resources (cfg->block_dict));
#endif
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

