#include <stdlib.h>
#include <stdio.h>

#include "http-status.h"

const char *
http_status_line (http_status status)
{
        switch (status) {
        case HTTP_STATUS_CONTINUE:
                return "HTTP/1.1 100 Continue\r\n";
                break;
        case HTTP_STATUS_SWITCHING_PROTOCOLS:
                return "HTTP/1.1 101 Switching Protocols\r\n";
                break;
        case HTTP_STATUS_OK:
                return "HTTP/1.1 200 OK\r\n";
                break;
        case HTTP_STATUS_CREATED:
                return "HTTP/1.1 201 Created\r\n";
                break;
        case HTTP_STATUS_ACCEPTED:
                return "HTTP/1.1 202 Accepted\r\n";
                break;
        case HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION:
                return "HTTP/1.1 203 Non-Authoritative Information\r\n";
                break;
        case HTTP_STATUS_NO_CONTENT:
                return "HTTP/1.1 204 No Content\r\n";
                break;
        case HTTP_STATUS_RESET_CONTENT:
                return "HTTP/1.1 205 Reset Content\r\n";
                break;
        case HTTP_STATUS_PARTIAL_CONTENT:
                return "HTTP/1.1 206 Partial Content\r\n";
                break;
        case HTTP_STATUS_MULTIPLE_CHOICES:
                return "HTTP/1.1 300 Multiple Choices\r\n";
                break;
        case HTTP_STATUS_MOVED_PERMANENTLY:
                return "HTTP/1.1 301 Moved Permanently\r\n";
                break;
        case HTTP_STATUS_FOUND:
                return "HTTP/1.1 302 Found\r\n";
                break;
        case HTTP_STATUS_SEE_OTHER:
                return "HTTP/1.1 303 See Other\r\n";
                break;
        case HTTP_STATUS_NOT_MODIFIED:
                return "HTTP/1.1 304 Not Modified\r\n";
                break;
        case HTTP_STATUS_USE_PROXY:
                return "HTTP/1.1 305 Use Proxy\r\n";
                break;
        case HTTP_STATUS_TEMPORARY_REDIRECT:
                return "HTTP/1.1 307 Temporary Redirect\r\n";
                break;
        case HTTP_STATUS_BAD_REQUEST:
                return "HTTP/1.1 400 Bad Request\r\n";
                break;
        case HTTP_STATUS_UNAUTHORIZED:
                return "HTTP/1.1 401 Unauthorized\r\n";
                break;
        case HTTP_STATUS_PAYMENT_REQUIRED:
                return "HTTP/1.1 402 Payment Required\r\n";
                break;
        case HTTP_STATUS_FORBIDDEN:
                return "HTTP/1.1 403 Forbidden\r\n";
                break;
        case HTTP_STATUS_NOT_FOUND:
                return "HTTP/1.1 404 Not Found\r\n";
                break;
        case HTTP_STATUS_METHOD_NOT_ALLOWED:
                return "HTTP/1.1 405 Method Not Allowed\r\n";
                break;
        case HTTP_STATUS_NOT_ACCEPTABLE:
                return "HTTP/1.1 406 Not Acceptable\r\n";
                break;
        case HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
                return "HTTP/1.1 407 Proxy Authentication Required\r\n";
                break;
        case HTTP_STATUS_REQUEST_TIMEOUT:
                return "HTTP/1.1 408 Request Time-out\r\n";
                break;
        case HTTP_STATUS_CONFLICT:
                return "HTTP/1.1 409 Conflict\r\n";
                break;
        case HTTP_STATUS_GONE:
                return "HTTP/1.1 410 Gone\r\n";
                break;
        case HTTP_STATUS_LENGTH_REQUIRED:
                return "HTTP/1.1 411 Length Required\r\n";
                break;
        case HTTP_STATUS_PRECONDITION_FAILED:
                return "HTTP/1.1 412 Precondition Failed\r\n";
                break;
        case HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE:
                return "HTTP/1.1 413 Request Entity Too Large\r\n";
                break;
        case HTTP_STATUS_REQUEST_URI_TOO_LARGE:
                return "HTTP/1.1 414 Request-URI Too Large\r\n";
                break;
        case HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
                return "HTTP/1.1 415 Unsupported Media Type\r\n";
                break;
        case HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE:
                return "HTTP/1.1 416 Requested range not satisfiable\r\n";
                break;
        case HTTP_STATUS_EXPECTATION_FAILED:
                return "HTTP/1.1 417 Expectation Failed\r\n";
                break;
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
                return "HTTP/1.1 500 Internal Server Error\r\n";
                break;
        case HTTP_STATUS_NOT_IMPLEMENTED:
                return "HTTP/1.1 501 Not Implemented\r\n";
                break;
        case HTTP_STATUS_BAD_GATEWAY:
                return "HTTP/1.1 502 Bad Gateway\r\n";
                break;
        case HTTP_STATUS_SERVICE_UNAVAILABLE:
                return "HTTP/1.1 503 Service Unavailable\r\n";
                break;
        case HTTP_STATUS_GATEWAY_TIMEOUT:
                return "HTTP/1.1 504 Gateway Time-out\r\n";
                break;
        case HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED:
                return "HTTP/1.1 505 HTTP Version not supported\r\n";
                break;
        default:
                return NULL;
                break;
        }
}

#define HTTP_STATUS_TMPL \
  "<html><head><title>%s</title></head><body><h1>%s</h1></body></html>\r\n"

params_t *
http_status_append_headers (params_t * response_headers, http_status status)
{
        char length[16];
        const char * status_line;
        char buf[1024];
        int n;

        response_headers = params_append (response_headers, "Content-Type", "text/html");

        n = strlen (http_status_line(status));
        snprintf (length, 16, "%d", strlen(HTTP_STATUS_TMPL)-4 + (n*2));
        response_headers = params_append (response_headers, "Content-Length", length);

        return response_headers;
}

int
http_status_stream_body (int fd, http_status status)
{
        const char * status_line;
        char buf[1024];
        int n;

        status_line = http_status_line (status);

        n = snprintf (buf, 1024, HTTP_STATUS_TMPL,
                      status_line, status_line);
        return write (fd, buf, n);
}
