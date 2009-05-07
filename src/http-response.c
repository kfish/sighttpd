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
#include "stream.h"
#include "flim.h"

/* #define DEBUG */

static void
respond (int fd, http_request * request, params_t * request_headers)
{
        params_t * response_headers = NULL;
        char date[256], headers_out[1024];
        const char * status_line;
        int status_request=0;
        int stream_request=0;

        status_line = http_status_line (HTTP_STATUS_OK);
        write (fd, status_line, strlen(status_line));

        status_request = !strncmp (request->path, "/status", 7);
        stream_request = !strncmp (request->path, "/stream", 7);

        httpdate_snprint (date, 256, time(NULL));
        response_headers = params_append (response_headers, "Date", date);
        response_headers = params_append (response_headers, "Server", "Sighttpd/" VERSION);

        if (status_request) {
                response_headers = status_append_headers (response_headers);
        } else if (stream_request) {
                response_headers = stream_append_headers (response_headers);
        } else {
                response_headers = flim_append_headers (response_headers);
        }

        params_snprint (headers_out, 1024, response_headers, PARAMS_HEADERS);
        write (fd, headers_out, strlen(headers_out));
        fsync (fd);

        log_access (request, request_headers, response_headers);

        if (status_request) {
                status_stream_body (fd);
        } else if (stream_request) {
                stream_stream_body (fd);
        } else {
                flim_stream_body (fd);
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
