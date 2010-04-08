#include <stdlib.h>
#include <string.h>

#include "cfg-read.h"
#include "cfg-parse.h"

static CopaStatus
cfg_read_block_start (const char * name, void * user_data)
{
  return COPA_OK;
}

static CopaStatus
cfg_read_block_end (const char * name, void * user_data)
{
  return COPA_OK;
}

static CopaStatus
cfg_read_assign (const char * name, const char * value, void * user_data)
{
  struct cfg * cfg = (struct cfg *) user_data;

  dictionary_insert (cfg->dictionary, name, value);

  return COPA_OK;
}

struct cfg *
cfg_read (const char * path)
{
  CopaStatus status;
  struct cfg * cfg;

  cfg = calloc (1, sizeof(*cfg));
  cfg->dictionary = dictionary_new ();

  status = copa_read (path, cfg_read_block_start, cfg,
		      cfg_read_block_end, cfg,
		      cfg_read_assign, cfg);

  if (status != COPA_OK) {
    free (cfg);
    return NULL;
  }

  return cfg;
}

