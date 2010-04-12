/*
   Copyright (C) 2009 Conrad Parker
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "shell.h"

#define UIOMUX_HEADER \
  "<html>\n" \
  "<head>\n" \
  "<title>Sighttpd/%s - UIOMux</title>\n" \
  "</head>\n" \
  "<body>\n" \
  "<h1>UIOMux status</h1>\n" \
  "<pre>"

#define UIOMUX_FOOTER \
  "</pre>\n" \
  "</body>\n" \
  "</head>\n"

static int
uiomux_check (http_request * request, void * data)
{
        return !strncmp (request->path, "/uiomux", 7);
}

static params_t *
uiomux_append_headers(params_t * response_headers)
{
	char length[16];

	fprintf (stderr, "%s: IN\n", __func__);

	response_headers =
	    params_append(response_headers, "Content-Type", "text/html");

	return response_headers;
}

static void
uiomux_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
        *status_line = http_status_line (HTTP_STATUS_OK);
        *response_headers = uiomux_append_headers (*response_headers);
}

static int
uiomux_body(int fd, http_request * request, params_t * request_headers, void * data)
{
	char buf[4096];
	int n, total = 0;
	int status;

	n = snprintf(buf, 4096, UIOMUX_HEADER, VERSION, VERSION);
	if (n < 0)
		return -1;
	if (n > 4096)
		n = 4096;
	total += write(fd, buf, n);

        total += shell_stream (fd, "uiomux info");
        total += shell_stream (fd, "uiomux meminfo");

	n = snprintf(buf, 4096, UIOMUX_FOOTER);
	if (n < 0)
		return -1;
	if (n > 4096)
		n = 4096;
	total += write(fd, buf, n);

        return total;
}

struct resource *
uiomux_resource (void)
{
	return resource_new (uiomux_check, uiomux_head, uiomux_body, NULL /* del */, NULL /* data */);
}
