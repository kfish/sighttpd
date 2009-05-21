
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "sighttpd.h"
#include "params.h"

#define STATUS_HEAD \
  "<html>\n" \
  "<head>\n" \
  "<title>Sighttpd/%s - Status</title>\n" \
  "</head>\n" \
  "<body>\n" \
  "<h1>Sighttpd/%s Status</h1>\n"

#define STATUS_FOOT \
  "</body>\n" \
  "</head>\n"

params_t *
status_append_headers (params_t * response_headers)
{
  char length[16];

  response_headers = params_append (response_headers, "Content-Type", "text/html");

  return response_headers;
}

int
status_stream_body (int fd, struct sighttpd * sighttpd)
{
    char buf[4096];
    int n, ntotal=0;

    n = snprintf (buf, 4096, STATUS_HEAD, VERSION, VERSION);
    if (n < 0) return -1;
    if (n > 4096) n = 4096;
    ntotal += n;
    write (fd, buf, n);

    n = snprintf (buf, 4096, "<p>Active streams: %d</p>\n", list_length (sighttpd->streams));
    if (n < 0) return -1;
    if (n > 4096) n = 4096;
    ntotal += n;
    write (fd, buf, n);
    

    n = snprintf (buf, 4096, STATUS_FOOT, VERSION, VERSION);
    if (n < 0) return -1;
    if (n > 4096) n = 4096;
    ntotal += n;
    write (fd, buf, n);

    return ntotal;
}
