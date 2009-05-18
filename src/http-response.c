#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "params.h"
#include "http-reqline.h"
#include "http-status.h"
#include "log.h"
#include "status.h"
#include "uiomux.h"
#include "stream.h"
#include "flim.h"

/* #define DEBUG */

static params_t *
response_headers_new (void)
{
        params_t * response_headers = NULL;
        char date[256];

        httpdate_snprint (date, 256, time(NULL));
        response_headers = params_append (response_headers, "Date", date);
        response_headers = params_append (response_headers, "Server", "Sighttpd/" VERSION);

        return response_headers;
}

static void
params_writefd (int fd, params_t * params)
{
        char headers_out[1024];

        params_snprint (headers_out, 1024, params, PARAMS_HEADERS);
        write (fd, headers_out, strlen(headers_out));
        fsync (fd);
}

static void
respond_get_head (http_request * request, params_t * request_headers,
                  const char ** status_line, params_t ** response_headers)
{
        int status_request=0;
        int uiomux_request=0;
        int stream_request=0;
        int kongou_request=0;
        int flim_request=0;

        status_request = !strncmp (request->path, "/status", 7);
        uiomux_request = !strncmp (request->path, "/uiomux", 7);
        stream_request = !strncmp (request->path, "/stream", 7);
        kongou_request = !strncmp (request->path, "/kongou", 7);
        flim_request = !strncmp (request->path, "/flim.txt", 9);

        if (status_request) {
                *status_line = http_status_line (HTTP_STATUS_OK);
                *response_headers = status_append_headers (*response_headers);
        } else if (uiomux_request) {
                *status_line = http_status_line (HTTP_STATUS_OK);
                *response_headers = uiomux_append_headers (*response_headers);
        } else if (kongou_request) {
                *status_line = http_status_line (HTTP_STATUS_OK);
                *response_headers = kongou_append_headers (*response_headers);
        } else if (stream_request) {
                *status_line = http_status_line (HTTP_STATUS_OK);
                *response_headers = stream_append_headers (*response_headers);
        } else if (flim_request) {
                *status_line = http_status_line (HTTP_STATUS_OK);
                *response_headers = flim_append_headers (*response_headers);
        } else {
                *status_line = http_status_line (HTTP_STATUS_NOT_FOUND);
                *response_headers = http_status_append_headers (*response_headers, HTTP_STATUS_NOT_FOUND);
        }
}

static void
respond_get_body (int fd, http_request * request, params_t * request_headers)
{
        int status_request=0;
        int uiomux_request=0;
        int stream_request=0;
        int kongou_request=0;
        int flim_request=0;

        status_request = !strncmp (request->path, "/status", 7);
        uiomux_request = !strncmp (request->path, "/uiomux", 7);
        stream_request = !strncmp (request->path, "/stream", 7);
        kongou_request = !strncmp (request->path, "/kongou", 7);
        flim_request = !strncmp (request->path, "/flim.txt", 9);

        if (status_request) {
                status_stream_body (fd);
        } else if (uiomux_request) {
                uiomux_stream_body (fd);
        } else if (kongou_request) {
                kongou_stream_body (fd, request->path);
        } else if (stream_request) {
                stream_stream_body (fd);
        } else if (flim_request) {
                flim_stream_body (fd);
        } else {
                http_status_stream_body (fd, HTTP_STATUS_NOT_FOUND);
        }
}

static void
respond_method_not_allowed (const char ** status_line, params_t ** response_headers)
{
        *status_line = http_status_line (HTTP_STATUS_METHOD_NOT_ALLOWED);
        *response_headers = params_append (*response_headers, "Allow", "GET");
        *response_headers = params_append (*response_headers, "Allow", "HEAD");
}

static void
respond (int fd, http_request * request, params_t * request_headers)
{
        params_t * response_headers;
        const char * status_line;

        response_headers = response_headers_new ();

        switch (request->method) {
        case HTTP_METHOD_HEAD:
        case HTTP_METHOD_GET:
                respond_get_head (request, request_headers, &status_line, &response_headers);
                break;
        default:
                respond_method_not_allowed (&status_line, &response_headers);
                break;
        }

        write (fd, status_line, strlen(status_line));
        params_writefd (fd, response_headers);

        log_access (request, request_headers, response_headers);

        if (request->method == HTTP_METHOD_GET) {
                respond_get_body (fd, request, request_headers);
        }

#ifdef DEBUG
        printf ("Finished serving / lost client\n");
#endif
}

void * http_response (void *arg)
{
        params_t * request_headers;
        http_request request;
        int fd = * (int *)arg;
	char s[1024];
        char * cur;
        size_t rem=1024, nread, n;
        int init=0;

        cur = s;

        setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, NULL, 0);

	/* proc client's requests */
	while ((nread = read(fd, cur, rem)) != 0) {
                if (n == -1) {
                        perror ("read");
                        continue;
                } else if (nread < 1024) {
                        /* NUL-terminate input buffer */
                        cur[nread] = '\0';
                }
                if (!init && (n = http_request_parse (cur, rem, &request)) > 0) {
                        memmove (s, &s[n], nread-n);
                        init = 1;
#ifdef DEBUG
                        printf ("Got HTTP method %d, version %d for %s (consumed %d)\n", request.method,
                                request.version, request.path, n);
#endif
                }

                request_headers = params_new_parse (s, strlen (s), PARAMS_HEADERS);
                if (request_headers != NULL) {
                        respond (fd, &request, request_headers);
                        goto closeit;
                } else {
                        n = strlen (cur);
                        cur += n;
                        rem -= n;
                }
	}

closeit:

	close(fd);		/* close the client's channel */

	return 0;		/* terminate the thread */
}
