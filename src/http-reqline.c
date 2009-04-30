#include <stdlib.h>
#include <string.h>

#include "http-reqline.h"

size_t
http_request_parse (char * s, size_t len, http_request * request)
{
        char * sp = " ";
        char * crlf = "\r\n";
        size_t span, consumed=0;
        char * original_reqline;
        http_method method;
        char * path;
        http_version version;

        original_reqline = s;

        /* Extract method */
        span = strcspn (s, sp);

        switch (span) {
        case 3:
                if (!strncmp (s, "GET", 3)) {
                        method = HTTP_METHOD_GET;
                } else if (!strncmp (s, "PUT", 3)) {
                        method = HTTP_METHOD_PUT;
                } else {
                        goto fail;
                }
                break;
        case 4:
                if (!strncmp (s, "HEAD", 4)) {
                        method = HTTP_METHOD_HEAD;
                } else if (!strncmp (s, "POST", 4)) {
                        method = HTTP_METHOD_POST;
                } else {
                        goto fail;
                }
                break;
        case 5:
                if (!strncmp (s, "TRACE", 5)) {
                        method = HTTP_METHOD_TRACE;
                } else {
                        goto fail;
                }
                break;
        case 6:
                if (!strncmp (s, "DELETE", 6)) {
                        method = HTTP_METHOD_DELETE;
                } else {
                        goto fail;
                }
                break;
        case 7:
                if (!strncmp (s, "OPTIONS", 7)) {
                        method = HTTP_METHOD_OPTIONS;
                } else if (!strncmp (s, "CONNECT", 7)) {
                        method = HTTP_METHOD_CONNECT;
                } else {
                        goto fail;
                }
                break;
        default:
                goto fail;
                break;
        }

        s += span+1;
        consumed += span+1;

        /* Extract path */
        span = strcspn (s, sp);
        path = malloc (span+1);
        strncpy (path, s, span);
        path[span] = '\0';

        s += span+1;
        consumed += span+1;

        /* Extract version */
        span = strcspn (s, crlf);
        if (span != 8) goto fail;
        if (!strncmp (s, "HTTP/1.1", 8)) {
                version = HTTP_VERSION_1_1;
        } else if (!strncmp (s, "HTTP/1.0", 8)) {
                version = HTTP_VERSION_1_0;
        } else if (!strncmp (s, "HTTP/0.9", 8)) {
                version = HTTP_VERSION_0_9;
        } else {
                goto fail;
        }

        s += span;
        consumed += span;

        span = strspn (s, crlf);
        if (span > 2) goto fail;

        request->original_reqline = strndup (original_reqline, consumed);
        consumed += span;

        request->method = method;
        request->path = path;
        request->version = version;

        return consumed;

fail:
        return 0;
}
