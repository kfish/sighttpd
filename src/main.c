/* gcc tcpserver-threaded.c -o tcpserver-threaded -lpthread */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>

#include "params.h"

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

char * test_headers =
  "Flim: flooey\r\n"
  "Host: camserve.cams.com\r\n"
  "\r\n";

void * http_response (void *arg)
{
        Params * request_headers;
	FILE *fp = (FILE *) arg;
	char s[1024], canon[1024];
        char * start;
        static char * cur = NULL;
        static size_t rem=1024;

        if (cur == NULL) cur = s;

	/* proc client's requests */
	while (fgets(cur, rem, fp) != 0 && strcmp(s, "bye\n") != 0) {
                if (!strcmp (cur, "GET /stream.m4v HTTP/1.1\r\n")) {
                        start = cur;
                } else {
                        puts ("========");
                        puts (start);
                        request_headers = params_new_parse (start, strlen (start), PARAMS_HEADERS);
                        if (request_headers != NULL) {
                                params_snprint (canon, 1024, request_headers, PARAMS_HEADERS);
                                fputs (canon, fp);
                                goto closeit;
                        } else {
                                cur += strlen (cur);
                        }
#if 0
		        printf("msg: %s", s);	/* display message */
		        fputs(s, fp);	/* echo it back */
#endif
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

        char pout[1024];
        Params * params;

        params = params_new_parse (test_headers, strlen (test_headers), PARAMS_HEADERS);
        params_snprint (pout, 1024, params, PARAMS_HEADERS);
        puts (pout);

        printf ("Heehee\n");

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
