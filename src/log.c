#include <stdio.h>

#include "params.h"
#include "http-reqline.h"

void log_access (http_request * request, Params * request_headers, Params * response_headers)
{
        char canon[1024];
        char * date, * user_agent, * content_length;

        /* Dump request headers to stdout */
        params_snprint (canon, 1024, request_headers, PARAMS_HEADERS);
        puts (canon);

        /* Apache-style logging */
        if ((date = params_get (response_headers, "Date")) == NULL)
            date = "";
        if ((user_agent = params_get (request_headers, "User-Agent")) == NULL)
            user_agent = "";
        if ((content_length = params_get (response_headers, "Content-Length")) == NULL)
            content_length = "";
        printf ("[%s] \"%s\" 200 %s \"%s\"\r\n", date, request->original_reqline,
                content_length, user_agent);
}

