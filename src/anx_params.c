/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "anx_compat.h"

#include <stdio.h>
#include <string.h>

#include <annodex/anx_list.h>

typedef enum {
  ANX_PARAMS_QUERY = 0,
  ANX_PARAMS_HEADERS = 1,
  ANX_PARAMS_METATAGS = 1000,
  ANX_PARAMS_PARAMTAGS = 1001
} AnxParamStyle;

typedef struct {
  char * key;
  char * value;
} AnxParam;

static AnxParam *
anx_param_new (char * key, char * value)
{
  AnxParam * p = anx_malloc (sizeof (AnxParam));
  p->key = strdup (key);
  p->value = strdup (value);
  return p;
}

static int
anx_snprint_params_format (char * buf, int n, AnxList * params, char * format)
{
  AnxList * l;
  AnxParam * p;
  int len, total = 0;

  for (l = params; l; l = l->next) {
    p = (AnxParam *)l->data;

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
anx_params_snprint (char * buf, size_t n, AnxList * params,
		    AnxParamStyle style)
{
  char * format = NULL;
  int len;

  switch (style) {
  case ANX_PARAMS_QUERY:
    format = "%s=%s&";
    break;
  case ANX_PARAMS_HEADERS:
    format = "%s: %s\r\n";
    break;
  case ANX_PARAMS_METATAGS:
    format = "<meta name=\"%s\" content=\"%s\"/>\n";
    break;
  case ANX_PARAMS_PARAMTAGS:
    format = "<param name=\"%s\" content=\"%s\"/>\n";
    break;
  default:
    return 0;
    break;
  }

  len = anx_snprint_params_format (buf, (int) n, params, format);

  if (style == ANX_PARAMS_QUERY) {
    if (len == -1) return -1;

    /* Remove trailing '&' after the last parameter */
    len--;

    if (len > (int)n) return len;
    buf[len] = '\0';
  }

  return len;
}

char *
anx_params_get (AnxList * params, char * key)
{
  AnxParam * p;
  AnxList * l;

  for (l = params; l; l = l->next) {
    p = (AnxParam *)l->data;
    if (!strcasecmp (p->key, key)) {
      return p->value;
    }
  }

  return NULL;
}

static AnxList *
anx_params_add (AnxList * params, char * key, char * value)
{
  AnxParam * p = anx_param_new (key, value);

  return anx_list_append (params, p);
}

AnxList *
anx_params_append (AnxList * params, char * key, char * value)
{
  AnxParam * p;
  AnxList * l;
  char * new_value;

  if (!key || !value) return params;

  for (l = params; l; l = l->next) {
    p = (AnxParam *)l->data;
    if (!strcasecmp (p->key, key)) {
      new_value = anx_malloc (strlen (p->value) + strlen (value) + 2);
      sprintf (new_value, "%s,%s", p->value, value);
      free (p->value);
      p->value = new_value;
      return params;
    }
  }

  return anx_params_add (params, key, value);
}

AnxList *
anx_params_replace (AnxList * params, char * key, char * value)
{
  AnxParam * p;
  AnxList * l;

  if (!key) return params;

  for (l = params; l; l = l->next) {
    p = (AnxParam *)l->data;
    if (!strcasecmp (p->key, key)) {
      free (p->value);
      p->value = strdup (value);
      return params;
    }
  }

  return anx_params_add (params, key, value);
}

AnxList *
anx_params_merge (AnxList * dest, AnxList * src)
{
  AnxParam * p;
  AnxList * l;
  
  for (l = src; l; l = l->next) {
    p = (AnxParam *)l->data;
    dest = anx_params_append (dest, p->key, p->value);
  }

  return dest;
}

static AnxList *
anx_params_new_parse_delim (char * input, char * val_delim, char * end_delim)
{
  char * key, * val, * end;
  AnxList * params = anx_list_new ();
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

    params = anx_params_append (params, key, val);

    key = end;

  } while (end != NULL);

  return params;
}

static AnxList *
anx_params_new_parse_query (char * query_string)
{
  char * cquery;
  AnxList * params;

  cquery = strdup (query_string);
  params = anx_params_new_parse_delim (cquery, "=", "&");
  free (cquery);

  return params;
}

static char *
anx_headers_canonicalize (char * headers)
{
  char * new_headers;
  char * c, * n;
  char * spht = " \t";
  char * crlf = "\r\n";
  char * lws = " \t\r\n";
  size_t span;

  new_headers = anx_malloc (strlen (headers) + 1);

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

static AnxList *
anx_params_new_parse_headers (char * headers)
{
  char * cheaders;
  AnxList * params;

  cheaders = anx_headers_canonicalize (headers);
  params = anx_params_new_parse_delim (cheaders, ": ", "\r\n");
  free (cheaders);

  return params;
}

AnxList *
anx_params_new_parse (char * input, AnxParamStyle style)
{
  switch (style) {
  case ANX_PARAMS_QUERY:
    return anx_params_new_parse_query (input);
  case ANX_PARAMS_HEADERS:
    return anx_params_new_parse_headers (input);
  default:
    return NULL;
  }
}

static void *
anx_param_clone (void * data)
{
  AnxParam * p = (AnxParam *)data;
  AnxParam * n = anx_param_new (p->key, p->value);
  return (void *)n;
}

AnxList *
anx_params_clone (AnxList * params)
{
  return anx_list_clone_with (params, anx_param_clone);
}

static void *
anx_param_free (void * data)
{
  AnxParam * p = (AnxParam *)data;

  free (p->key);
  free (p->value);
  free (p);

  return NULL;
}

AnxList *
anx_params_remove (AnxList * params, char * key)
{
  AnxParam * p;
  AnxList * l;

  for (l = params; l; l = l->next) {
    p = (AnxParam *)l->data;
    if (!strcmp (p->key, key)) {
      params = anx_list_remove (params, l);
      anx_param_free (p);
      anx_free (l);
      return params;
    }
  }

  return params;
}

AnxList *
anx_params_free (AnxList * params)
{
  return anx_list_free_with (params, anx_param_free);
}

