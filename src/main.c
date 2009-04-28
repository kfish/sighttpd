/* gcc tcpserver-threaded.c -o tcpserver-threaded -lpthread */

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
#include "status.h"
#include "flim.h"

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

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

void respond (FILE * fp, http_request * request, Params * request_headers)
{
        Params * response_headers = NULL;
        char date[256], headers_out[1024];
        int status_request=0;

        fputs ("HTTP/1.1 200 OK\r\n", fp);

        status_request = !strncmp (request->path, "/status", 7);

        httpdate_snprint (date, 256, time(NULL));
        response_headers = params_append (response_headers, "Date", date);
        response_headers = params_append (response_headers, "Server", "Sighttpd/" VERSION);

        if (status_request) {
                response_headers = status_append_headers (response_headers);
        } else {
                response_headers = flim_append_headers (response_headers);
        }

        params_snprint (headers_out, 1024, response_headers, PARAMS_HEADERS);
        fputs (headers_out, fp);

        if (status_request) {
                status_stream_body (fp);
        } else {
                flim_stream_body (fp);
        }

        log_access (request, request_headers, response_headers);
}

void * http_response (void *arg)
{
        Params * request_headers;
        http_request request;
	FILE *fp = (FILE *) arg;
	char s[1024];
        char * start;
        static char * cur = NULL;
        static size_t rem=1024;
        int init=0;

        if (cur == NULL) cur = s;

	/* proc client's requests */
	while (fgets(cur, rem, fp) != 0 && strcmp(s, "bye\n") != 0) {
                if (!init && http_request_parse (cur, rem, &request) > 0) {
                        start = cur;
                        init = 1;
                        printf ("Got HTTP method %d, version %d for %s\n", request.method,
                                request.version, request.path);
                } else {
                        request_headers = params_new_parse (start, strlen (start), PARAMS_HEADERS);
                        if (request_headers != NULL) {
                                respond (fp, &request, request_headers);
                                goto closeit;
                        } else {
                                cur += strlen (cur);
                        }
                }
	}

closeit:

	fclose(fp);		/* close the client's channel */

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
		FILE *fp;

		/* process all incoming clients */
		while (1) {
			size = sizeof(struct sockaddr_in);
			ad = accept(sd, &gotcha, &size);

			if (ad == -1) {
				perror("accept");
			} else {
				fp = fdopen(ad, "r+");	/* convert into FILE* */
				pthread_create(&child, 0,  http_response , fp);	/* start thread */
				pthread_detach(child);	/* don't track it */
			}
		}
	}
}
