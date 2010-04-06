#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"

#define FLIM_TEXT "flim flim flim\n"

static int
flim_check (http_request * request, void * data)
{
        return !strncmp (request->path, "/flim.txt", 9);
}

static params_t *
flim_append_headers (params_t * response_headers)
{
        char length[16];

        response_headers = params_append (response_headers, "Content-Type", "text/plain");
        snprintf (length, 16, "%d", strlen (FLIM_TEXT));
        response_headers = params_append (response_headers, "Content-Length", length);

	return response_headers;
}

static void
flim_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
        *status_line = http_status_line (HTTP_STATUS_OK);
        *response_headers = flim_append_headers (*response_headers);
}

static void
flim_body (int fd, http_request * request, params_t * request_headers, void * data)
{
        write (fd, FLIM_TEXT, strlen(FLIM_TEXT));
}

struct resource *
flim_resource (void)
{
	return resource_new (flim_check, flim_head, flim_body, NULL);
}
