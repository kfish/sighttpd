#include <stdio.h>
#include <string.h>

#include "params.h"

#define KONGOU_TEXT "kongou kongou kongou\n"

params_t *
kongou_append_headers (params_t * response_headers)
{
        char length[16];

        response_headers = params_append (response_headers, "Content-Type", "text/plain");
        snprintf (length, 16, "%d", strlen (KONGOU_TEXT));
        response_headers = params_append (response_headers, "Content-Length", length);
}

int
kongou_stream_body (int fd)
{
        write (fd, KONGOU_TEXT, strlen(KONGOU_TEXT));
}

