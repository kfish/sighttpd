
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"
#include "sighttpd.h"

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

static int
status_check (http_request * request, void * data)
{
        return !strncmp (request->path, "/status", 7);
}

static params_t *
status_append_headers (params_t * response_headers)
{
  char length[16];

  response_headers = params_append (response_headers, "Content-Type", "text/html");

  return response_headers;
}

static void
status_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
        *status_line = http_status_line (HTTP_STATUS_OK);
        *response_headers = status_append_headers (*response_headers);
}

static int
status_body (int fd, http_request * request, params_t * request_headers, void * data)
{
    char buf[4096];
    int n, ntotal=0;
    struct sighttpd * sighttpd = (struct sighttpd *)data;

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

struct resource *
status_resource (struct sighttpd * sighttpd)
{
	return resource_new (status_check, status_head, status_body, NULL /* del */, sighttpd);
}
