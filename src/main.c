#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>

#include "params.h"
#include "http-reqline.h"
#include "log.h"

#include "status.h"
#include "stream.h"
#include "flim.h"

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

#define STATUS_OK "HTTP/1.1 200 OK\r\n"

void respond (int fd, http_request * request, params_t * request_headers)
{
        params_t * response_headers = NULL;
        char date[256], headers_out[1024];
        int status_request=0;
        int stream_request=0;

        write (fd, STATUS_OK, strlen(STATUS_OK));

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

        log_access (request, request_headers, response_headers);

        if (status_request) {
                status_stream_body (fd);
        } else if (stream_request) {
                stream_stream_body (fd);
        } else {
                flim_stream_body (fd);
        }

        printf ("Finished serving / lost client\n");
}

void * http_response (void *arg)
{
        params_t * request_headers;
        http_request request;
        int fd = * (int *)arg;
	char s[1024];
        char * start;
        static char * cur = NULL;
        static size_t rem=1024;
        int init=0;

        if (cur == NULL) cur = s;

	/* proc client's requests */
	while (read(fd, cur, rem) > 0) {
                if (!init && http_request_parse (cur, rem, &request) > 0) {
                        start = cur;
                        init = 1;
                        printf ("Got HTTP method %d, version %d for %s\n", request.method,
                                request.version, request.path);
                } else {
                        request_headers = params_new_parse (start, strlen (start), PARAMS_HEADERS);
                        if (request_headers != NULL) {
                                respond (fd, &request, request_headers);
                                goto closeit;
                        } else {
                                cur += strlen (cur);
                        }
                }
	}

closeit:

	close(fd);		/* close the client's channel */

	return 0;		/* terminate the thread */
}

int main(int count, char *args[])
{
	struct sockaddr_in addr;
	int sd, port;

	if (count != 2) {
		printf("usage: %s <protocol or portnum>\n", args[0]);
		exit(0);
	}

	/* Get server's IP and standard service connection* */
	if (!isdigit(args[1][0])) {
		struct servent *srv = getservbyname(args[1], "tcp");

		if (srv == NULL)
			panic(args[1]);

		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	} else {
		port = htons(atoi(args[1]));
	}

	/* Create socket * */
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if (sd < 0)
		panic("socket");

	/* Bind port/address to socket * */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;	/* any interface */

	if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) != 0)
		panic("bind");

	/* Make into listener with 10 slots * */
	if (listen(sd, 10) != 0) {
		panic("listen")
	} else {
		/* Begin waiting for connections * */
		int ad;
		socklen_t size;
		struct sockaddr gotcha;
		pthread_t child;

                log_open ();

                stream_init ();

		/* process all incoming clients */
		while (1) {
			size = sizeof(struct sockaddr_in);
			ad = accept(sd, &gotcha, &size);

			if (ad == -1) {
				perror("accept");
			} else {
				pthread_create(&child, 0,  http_response , &ad);	/* start thread */
				pthread_detach(child);	/* don't track it */
			}
		}

                stream_close ();
                log_close ();
	}
}
