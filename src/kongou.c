#include <stdio.h>
#include <string.h>

#include "params.h"

#define KONGOU_TEXT "kongou kongou kongou\n"

#define TTYSC 5

params_t *
kongou_append_headers (params_t * response_headers)
{
        response_headers = params_append (response_headers, "Content-Type", "text/plain");
}

int
kongou_stream_body (int fd, char * path)
{
        char *q;
        char buf[1024], cmd[64];
        params_t * query;
        char * wb_p;
        int wb=0;
        size_t n;

        q = index (path, '?');

        if (q == NULL) {
                n = snprintf (buf, 1024, "kongou!! %s\n", path);
        } else {
                q++;
                query = params_new_parse (q, strlen(q), PARAMS_QUERY);
 
                wb_p = params_get (query, "wb");
                if (wb_p)
                        wb = atoi(wb_p);
        
                n = snprintf (cmd, 64, "kgctrl set %d %d %d", TTYSC, 18 /* WB */, wb);
                n = snprintf (buf, 1024, "Set white balance to %d: %s\n", wb, cmd);

        }
        write (fd, buf, n);
}

