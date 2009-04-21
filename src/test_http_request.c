/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"

#include "tests.h"

static void
test_http_parse (char * s, http_method e_method, char * e_path, http_version e_version)
{
        char buf[256];
        http_request request;
        size_t len;

        strncpy (buf, s, strlen(s));
        buf[strlen(s)-2] = '\0';
        INFO (buf);

        len = http_request_parse (s, strlen(s), &request);
        if (len != strlen (s)) {
                FAIL ("Did not consume entire line");
        }
}

int
main (int argc, char * argv[])
{
        INFO ("Testing parse of valid HTTP request lines:");

        test_http_parse ("GET /stream.ogv HTTP/1.1\r\n", HTTP_METHOD_GET, "/stream.ogv", HTTP_VERSION_1_1);
        test_http_parse ("PUT /stream.ogv HTTP/1.1\r\n", HTTP_METHOD_PUT, "/stream.ogv", HTTP_VERSION_1_1);
        test_http_parse ("OPTIONS * HTTP/1.1\r\n", HTTP_METHOD_OPTIONS, "*", HTTP_VERSION_1_1);
                
        exit (EXIT_SUCCESS);
}
