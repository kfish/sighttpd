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
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "params.h"
#include "http-reqline.h"
#include "log.h"
#include "status.h"
#include "stream.h"
#include "flim.h"

/* #define DEBUG */

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

#define STATUS_OK "HTTP/1.1 200 OK\r\n"

static char * progname;

static void
usage (const char * progname)
{
        //printf ("Usage: %s [options] <port>\n", progname);
        printf ("Usage: %s <port>\n", progname);
        printf ("A stream ingress HTTP server.\n");
        printf ("\n");
        printf ("Please report bugs to <" PACKAGE_BUGREPORT ">\n");
}

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

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int sd, port;

        progname = argv[0];

	if (argc != 2) {
                usage (progname);
		return (1);
	}

	/* Get server's IP and standard service connection* */
	if (!isdigit(argv[1][0])) {
		struct servent *srv = getservbyname(argv[1], "tcp");

		if (srv == NULL)
			panic(argv[1]);

		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	} else {
		port = htons(atoi(argv[1]));
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
                pthread_attr_t attr;

                /* set thread create attributes */
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                /* Ignore SIGPIPE, handle client disconnect in processing threads */
                signal(SIGPIPE, SIG_IGN);

                log_open ();

                stream_init ();

		/* process all incoming clients */
		while (1) {
			size = sizeof(struct sockaddr_in);
			ad = accept(sd, &gotcha, &size);

			if (ad == -1) {
				perror("accept");
			} else {
				pthread_create(&child, &attr,  http_response , &ad);	/* start thread */
			}
		}

                stream_close ();
                log_close ();
	}
}
