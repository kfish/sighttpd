/*
   Copyright (C) 2009 Conrad Parker
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

typedef enum {
  PARAMS_QUERY = 0,
  PARAMS_HEADERS = 1,
  PARAMS_METATAGS = 1000,
  PARAMS_PARAMTAGS = 1001
} ParamStyle;

typedef struct {
  char * key;
  char * value;
} Params;

static Params *
param_new (char * key, char * value)
{
  Params * p = malloc (sizeof (Params));
  p->key = strdup (key);
  p->value = strdup (value);
  return p;
}

static int
snprint_params_format (char * buf, int n, List * params, char * format)
{
  List * l;
  Params * p;
  int len, total = 0;

  for (l = params; l; l = l->next) {
    p = (Params *)l->data;

    len = snprintf (buf, n, format, p->key, p->value);

    /* drop out if non-C99 return value */
    if (len < 0) return -1;
    
    if (len <= n) {
      n -= len;
      buf += len;
    } else {
      n = 0;
      buf = NULL;
    }

    total += len;
  }

  return total;
}

int
params_snprint (char * buf, size_t n, List * params,
		    ParamStyle style)
{
  char * format = NULL;
  int len;

  switch (style) {
  case PARAMS_QUERY:
    format = "%s=%s&";
    break;
  case PARAMS_HEADERS:
    format = "%s: %s\r\n";
    break;
  case PARAMS_METATAGS:
    format = "<meta name=\"%s\" content=\"%s\"/>\n";
    break;
  case PARAMS_PARAMTAGS:
    format = "<param name=\"%s\" content=\"%s\"/>\n";
    break;
  default:
    return 0;
    break;
  }

  len = snprint_params_format (buf, (int) n, params, format);

  if (style == PARAMS_QUERY) {
    if (len == -1) return -1;

    /* Remove trailing '&' after the last parameter */
    len--;

    if (len > (int)n) return len;
    buf[len] = '\0';
  }

  return len;
}

char *
params_get (List * params, char * key)
{
  Params * p;
  List * l;

  for (l = params; l; l = l->next) {
    p = (Params *)l->data;
    if (!strcasecmp (p->key, key)) {
      return p->value;
    }
  }

  return NULL;
}

static List *
params_add (List * params, char * key, char * value)
{
  Params * p = param_new (key, value);

  return list_append (params, p);
}

List *
params_append (List * params, char * key, char * value)
{
  Params * p;
  List * l;
  char * new_value;

  if (!key || !value) return params;

  for (l = params; l; l = l->next) {
    p = (Params *)l->data;
    if (!strcasecmp (p->key, key)) {
      new_value = malloc (strlen (p->value) + strlen (value) + 2);
      sprintf (new_value, "%s,%s", p->value, value);
      free (p->value);
      p->value = new_value;
      return params;
    }
  }

  return params_add (params, key, value);
}

List *
params_replace (List * params, char * key, char * value)
{
  Params * p;
  List * l;

  if (!key) return params;

  for (l = params; l; l = l->next) {
    p = (Params *)l->data;
    if (!strcasecmp (p->key, key)) {
      free (p->value);
      p->value = strdup (value);
      return params;
    }
  }

  return params_add (params, key, value);
}

List *
params_merge (List * dest, List * src)
{
  Params * p;
  List * l;
  
  for (l = src; l; l = l->next) {
    p = (Params *)l->data;
    dest = params_append (dest, p->key, p->value);
  }

  return dest;
}

static List *
params_new_parse_delim (char * input, char * val_delim, char * end_delim)
{
  char * key, * val, * end;
  List * params = list_new ();
  size_t span;

  if (!input) return params;

  key = input;
  
  do {
    span = strcspn (key, val_delim);
    val = span > 0 ? key + span : NULL;

    span = strcspn (key, end_delim);
    end = span > 0 ? key + span : NULL;

    if (end) {
      if (val) {
        if (val < end) {
	  span = strspn (val, val_delim);
	  memset (val, '\0', span);
	  val += span;
        } else {
          val = NULL;
        }
      }
      span = strspn (end, end_delim);
      memset (end, '\0', span);
      end += span;
    } else {
      if (val) *val++ = '\0';
    }

    params = params_append (params, key, val);

    key = end;

  } while (end != NULL);

  return params;
}

static List *
params_new_parse_query (char * query_string)
{
  char * cquery;
  List * params;

  cquery = strdup (query_string);
  params = params_new_parse_delim (cquery, "=", "&");
  free (cquery);

  return params;
}

static char *
headers_canonicalize (char * headers)
{
  char * new_headers;
  char * c, * n;
  char * spht = " \t";
  char * crlf = "\r\n";
  char * lws = " \t\r\n";
  size_t span;

  new_headers = malloc (strlen (headers) + 1);

  c = headers;
  n = new_headers;

  while (*c) {
    span = strcspn (c, lws);
    memcpy (n, c, span);
    n += span;
    c += span;

    /* collapse a sequence of SP and HT into a single SP */
    span = strspn (c, spht);
    if (span > 0) {
      c += span;
      *n++ = ' ';
    }

    /* collapse a sequence of CR and LF into a single CRLF,
       unless the next line begins with SP or HT, in which
       case collapse the entire LWS sequence into a single SP
    */
    span = strspn (c, crlf);
    if (span > 0) {
      c += span;
      span = strspn (c, spht);
      if (span > 0) {
	c += span;
	*n++ = ' ';
      } else {
	*n++ = '\r';
	*n++ = '\n';
      }
    }
  }

  *n = '\0';

  return new_headers;
}

static List *
params_new_parse_headers (char * headers)
{
  char * cheaders;
  List * params;

  cheaders = headers_canonicalize (headers);
  params = params_new_parse_delim (cheaders, ": ", "\r\n");
  free (cheaders);

  return params;
}

List *
params_new_parse (char * input, ParamStyle style)
{
  switch (style) {
  case PARAMS_QUERY:
    return params_new_parse_query (input);
  case PARAMS_HEADERS:
    return params_new_parse_headers (input);
  default:
    return NULL;
  }
}

static void *
param_free (void * data)
{
  Params * p = (Params *)data;

  free (p->key);
  free (p->value);
  free (p);

  return NULL;
}

List *
params_remove (List * params, char * key)
{
  Params * p;
  List * l;

  for (l = params; l; l = l->next) {
    p = (Params *)l->data;
    if (!strcmp (p->key, key)) {
      params = list_remove (params, l);
      param_free (p);
      free (l);
      return params;
    }
  }

  return params;
}

List *
params_free (List * params)
{
  return list_free_with (params, param_free);
}

