#ifndef __CFG_READ_H__
#define __CFG_READ_H__

#include "dictionary.h"

struct cfg {
  Dictionary * dictionary;
};

struct cfg * cfg_read (const char * path);

#endif /* __CFG_READ_H__ */
