
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "params.h"

#define STATUS_TMPL \
  "<html>\n" \
  "<head>\n" \
  "<title>Sighttpd/%s - Status</title>\n" \
  "</head>\n" \
  "<body>\n" \
  "<h1>Sighttpd/%s Status</h1>\n" \
  "<p>OK</p>\n" \
  "</body>\n" \
  "</head>\n"

params_t *
status_append_headers (params_t * response_headers)
{
  char length[16];

  response_headers = params_append (response_headers, "Content-Type", "text/html");

  snprintf (length, 16, "%d", strlen(STATUS_TMPL)-4 + (strlen(VERSION)*2));
  response_headers = params_append (response_headers, "Content-Length", length);

  return response_headers;
}

int
status_stream_body (FILE * stream)
{
    fprintf (stream, STATUS_TMPL, VERSION, VERSION);
}
