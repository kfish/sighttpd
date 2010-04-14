#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <signal.h>

#include "dictionary.h"
#include "http-response.h"
#include "sighttpd.h"
#include "cfg-read.h"

#ifdef HAVE_SHCODECS
#include "shrecord.h"
#endif

/* #define DEBUG */

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

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

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int sd;
        struct sighttpd * sighttpd;
        struct cfg * cfg;

        progname = argv[0];

#ifdef HAVE_SHCODECS
	shrecord_init();
#endif

        cfg = cfg_read ("/etc/sighttpd/sighttpd.conf");

        if (argc == 2) {
		dictionary_insert (cfg->dictionary, "Listen", argv[1]);
	}

        log_open ();

        sighttpd = sighttpd_init (cfg);

	free (cfg);

#ifdef HAVE_SHCODECS
	shrecord_run();
#endif
	/* Create socket * */
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if (sd < 0)
		panic("socket");

	/* Bind port/address to socket * */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(sighttpd->port);
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
                struct sighttpd_child * schild;

                /* set thread create attributes */
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                /* Ignore SIGPIPE, handle client disconnect in processing threads */
                signal(SIGPIPE, SIG_IGN);

		/* process all incoming clients */
		while (1) {
			size = sizeof(struct sockaddr_in);
			ad = accept(sd, &gotcha, &size);

			if (ad == -1) {
				perror("accept");
			} else {
                                schild = sighttpd_child_new (sighttpd, ad);
				pthread_create(&child, &attr, http_response, schild);	/* start thread */
			}
		}

	}

        log_close ();
}
