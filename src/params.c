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
} params_style;

typedef struct {
  char * key;
  char * value;
} params_t;

static params_t *
param_new (char * key, char * value)
{
  params_t * p = malloc (sizeof (params_t));

  p->key = strdup (key);
  p->value = strdup (value);
  return p;
}

static size_t
snprint_params_format (char * buf, size_t n, list_t * params, char * format)
{
  list_t * l;
  params_t * p;
  size_t len, total = 0;

  for (l = params; l; l = l->next) {
    p = (params_t *)l->data;

    len = (size_t)snprintf (buf, n, format, p->key, p->value);

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
params_snprint (char * buf, size_t n, list_t * params,
                params_style style)
{
  char * format = NULL;
  size_t len;

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

  len = snprint_params_format (buf, n, params, format);
  if (len == -1) return -1;

  if (style == PARAMS_QUERY) {
    /* Remove trailing '&' after the last parameter */
    len--;

    if (len <= n)
      buf[len] = '\0';
  } else if (style == PARAMS_HEADERS) {
    /* Add extra trailing CRLF */
    if (n > 0 && len < n-2) {
      buf[len++] = '\r';
      buf[len++] = '\n';
      buf[len] = '\0';
    } else {
      len += 2;
    }
  }

  return len;
}

char *
params_get (list_t * params, char * key)
{
  params_t * p;
  list_t * l;

  for (l = params; l; l = l->next) {
    p = (params_t *)l->data;
    if (!strcasecmp (p->key, key)) {
      return p->value;
    }
  }

  return NULL;
}

static list_t *
params_add (list_t * params, char * key, char * value)
{
  params_t * p = param_new (key, value);

  return list_append (params, p);
}

list_t *
params_append (list_t * params, char * key, char * value)
{
  params_t * p;
  list_t * l;
  char * new_value;

  if (!key || !value) return params;

  for (l = params; l; l = l->next) {
    p = (params_t *)l->data;
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

list_t *
params_replace (list_t * params, char * key, char * value)
{
  params_t * p;
  list_t * l;

  if (!key) return params;

  for (l = params; l; l = l->next) {
    p = (params_t *)l->data;
    if (!strcasecmp (p->key, key)) {
      free (p->value);
      p->value = strdup (value);
      return params;
    }
  }

  return params_add (params, key, value);
}

list_t *
params_merge (list_t * dest, list_t * src)
{
  params_t * p;
  list_t * l;
  
  for (l = src; l; l = l->next) {
    p = (params_t *)l->data;
    dest = params_append (dest, p->key, p->value);
  }

  return dest;
}

/* Requires NULL-terminated input, as guaranteed by query and headers
 * parsers below */
static list_t *
params_new_parse_delim (char * input, char * val_delim, char * end_delim)
{
  char * key, * val, * end;
  list_t * params = list_new ();
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

    if (val && end) {
      params = params_append (params, key, val);
      key = end;
    }

  } while (end != NULL);

  return params;
}

static list_t *
params_new_parse_query (char * query_string, size_t len)
{
  char * cquery;
  list_t * params;

  cquery = strndup (query_string, len);
  params = params_new_parse_delim (cquery, "=", "&");
  free (cquery);

  return params;
}

/*
 * Canonicalize HTTP-style headers, but return NULL if they are not
 * terminated by CRLFCRLF.
 */
static char *
headers_canonicalize (char * headers, size_t len)
{
  char * new_headers;
  char * c, * n, * eol;
  char * spht = " \t";
  char * crlf = "\r\n";
  char * lws = " \t\r\n";
  size_t span;
  int nr_cr, nr_lf, i;
  int success = 0;

  new_headers = malloc (strlen (headers) + 1);

  c = headers;
  n = new_headers;

  while (*c) {
    /* Skip non-whitespace */
    span = strcspn (c, lws);
    memcpy (n, c, span);
    n += span;
    c += span;

    /* Collapse a sequence of SP and HT into a single SP */
    span = strspn (c, spht);
    if (span > 0) {
      c += span;
      *n++ = ' ';
    }

    eol = c;

    /* collapse a sequence of CR and LF into a single CRLF,
       unless the next line begins with SP or HT, in which
       case collapse the entire LWS sequence into a single SP
    */
    span = strspn (c, crlf);
    if (span > 0) {
      nr_cr = 0; nr_lf = 0;
      for (i = 0; i < span; i++) {
        if (eol[i] == '\r') nr_cr++;
        if (eol[i] == '\n') nr_lf++;
      }
      
      /* Break out successfully if this block of CRLF contains
       * 2 or more of either of those characters */
      if (nr_cr >= 2 || nr_lf >= 2) {
          success = 1;
          break;
      }
      
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

  if (!success) {
    free (new_headers);
    new_headers = NULL;
  }

  return new_headers;
}

static list_t *
params_new_parse_headers (char * headers, size_t len)
{
  char * cheaders;
  list_t * params;

  cheaders = headers_canonicalize (headers, len);
  if (cheaders == NULL) return NULL;

  params = params_new_parse_delim (cheaders, ": ", "\r\n");
  free (cheaders);

  return params;
}

list_t *
params_new_parse (char * input, size_t len, params_style style)
{
  switch (style) {
  case PARAMS_QUERY:
    return params_new_parse_query (input, len);
  case PARAMS_HEADERS:
    return params_new_parse_headers (input, len);
  default:
    return NULL;
  }
}

static void *
param_free (void * data)
{
  params_t * p = (params_t *)data;

  free (p->key);
  free (p->value);
  free (p);

  return NULL;
}

list_t *
params_remove (list_t * params, char * key)
{
  params_t * p;
  list_t * l;

  for (l = params; l; l = l->next) {
    p = (params_t *)l->data;
    if (!strcmp (p->key, key)) {
      params = list_remove (params, l);
      param_free (p);
      free (l);
      return params;
    }
  }

  return params;
}

list_t *
params_free (list_t * params)
{
  return list_free_with (params, param_free);
}

