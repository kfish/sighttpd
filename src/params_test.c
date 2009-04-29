/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"

#include "tests.h"

static char * h =
"Fish-Type: haddock\r\n"
"Cheese-Age: mouldy\r\n"
"Moose-Flavor: red\r\n"
"Cheese-Age: green\r\n"
"\r\n";

static char * q =
"Fish-Type=haddock&Cheese-Age=mouldy&Moose-Flavor=red&Cheese-Age=green";

static char * m =
"Fish-Type=bream";

static char * h_header =
"Fish-Type: haddock\r\n"
"Cheese-Age: mouldy,green\r\n"
"Moose-Flavor: red\r\n"
"\r\n";

static char * h_query =
"Fish-Type=haddock&Cheese-Age=mouldy,green&Moose-Flavor=red";

static char * h_meta =
"<meta name=\"Fish-Type\" content=\"haddock\"/>\n"
"<meta name=\"Cheese-Age\" content=\"mouldy,green\"/>\n"
"<meta name=\"Moose-Flavor\" content=\"red\"/>\n";

static char * h_param =
"<param name=\"Fish-Type\" content=\"haddock\"/>\n"
"<param name=\"Cheese-Age\" content=\"mouldy,green\"/>\n"
"<param name=\"Moose-Flavor\" content=\"red\"/>\n";

static void
test_print (params_t * params, params_style style, int correct_len,
	    char * correct_output)
{
  char buf[1024];
  int len;

  len = params_snprint (NULL, 0, params, style);
  if (len <= 0) {
    WARN ("system has non-C99 snprintf");
  } else if (len != correct_len) {
    FAIL ("incorrect print length");
  }

  memset (buf, '\0', 1024);
  len = params_snprint (buf, 1024, params, style);
  if (len != correct_len)
    FAIL ("incorrect print length");
  if (strcmp (buf, correct_output))
    FAIL ("incorrect printing");
}

static void
test_params (params_t * params)
{
  params_t * m_params = NULL;
  char * v;

  INFO ("  get value");
  v = params_get (params, "Fish-Type");
  if (strcmp (v, "haddock"))
    FAIL ("error storing param values");

  INFO ("  get appended value");
  v = params_get (params, "Cheese-Age");
  if (strcmp (v, "mouldy,green"))
    FAIL ("error appending param values");

  INFO ("  print query");
  test_print (params, PARAMS_QUERY, 58, h_query);

  INFO ("  print headers");
  test_print (params, PARAMS_HEADERS, 67, h_header);

  INFO ("  print metatags");
  test_print (params, PARAMS_METATAGS, 134, h_meta);

  INFO ("  print paramtags");
  test_print (params, PARAMS_PARAMTAGS, 137, h_param);

  INFO ("  remove value");
  params = params_remove (params, "Cheese-Age");
  v = params_get (params, "Cheese-Age");
  if (v)
    FAIL ("error removing param value");

  INFO ("  replace value");
  params = params_replace (params, "Moose-Flavor", "curry");
  v = params_get (params, "Moose-Flavor");
  if (strcmp (v, "curry"))
    FAIL ("error replacing param values");

  INFO ("  merge params");
  m_params = params_new_parse (m, strlen(m), PARAMS_QUERY);
  params = params_merge (params, m_params);
  v = params_get (params, "Fish-Type");
  if (strcmp (v, "haddock,bream"))
    FAIL ("error merging params");

  INFO ("  free params");
  params_free (m_params);
  params_free (params);
}

int
main (int argc, char * argv[])
{
  params_t * params;

  INFO ("Testing param generation");
  params = NULL;
  params = params_append (params, "Fish-Type", "haddock");
  params = params_append (params, "Cheese-Age", "mouldy");
  params = params_append (params, "Moose-Flavor", "red");
  params = params_append (params, "Cheese-Age", "green");
  test_params (params);

  INFO ("Testing query parsing");
  params = params_new_parse (q, strlen(q), PARAMS_QUERY);
  test_params (params);

  INFO ("Testing header parsing");
  params = params_new_parse (h, strlen(h), PARAMS_HEADERS);
  if (params == NULL)
    FAIL ("parsing headers");
  test_params (params);

  exit (EXIT_SUCCESS);
}
