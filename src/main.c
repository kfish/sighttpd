/*
   Copyright (C) 2009 Conrad Parker
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <signal.h>

#include "dictionary.h"
#include "http-response.h"
#include "sighttpd.h"
#include "cfg-read.h"

#ifdef HAVE_OGGZ
#include "ogg-stdin.h"
#endif

#ifdef HAVE_SHCODECS
#include "shrecord.h"
#endif

/* #define DEBUG */

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

#define DEFAULT_CONFIG_FILENAME "/etc/sighttpd/sighttpd.conf"

static char * progname;

static char * optstring = "f:hv";

#ifdef HAVE_GETOPT_LONG
static struct option long_options[] = {
	{ "config", required_argument, NULL, 'f'},
	{ "help", no_argument, 0, 'h'},
	{ "version", no_argument, 0, 'v'},
};
#endif

static void
usage (const char * progname)
{
        printf ("Usage: %s [options] <port>\n", progname);
        printf ("A stream ingress HTTP server.\n");
	printf ("\nConfiguration options\n");
	printf ("  -f filename       Specify configuration filename [/etc/sighttpd/sighttpd.conf]\n");
        printf ("\nPlease report bugs to <" PACKAGE_BUGREPORT ">\n");
}

void sig_handler(int sig)
{
#ifdef HAVE_OGGZ
	oggstdin_sighandler ();
#endif

#ifdef HAVE_SHCODECS
        shrecord_sighandler ();
#endif

#ifdef DEBUG
        fprintf (stderr, "Got signal %d\n", sig);
#endif

        /* Send ourselves the signal: see http://www.cons.org/cracauer/sigint.html */
        signal(sig, SIG_DFL);
        kill(getpid(), sig);
}


int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int sd;
        struct sighttpd * sighttpd;
        struct cfg * cfg;
	char * config_filename = DEFAULT_CONFIG_FILENAME;
	int c;
	int show_version = 0;
	int show_help = 0;

        progname = argv[0];

#ifdef HAVE_SHCODECS
	shrecord_init();
#endif

	while (1) {
#ifdef HAVE_GETOPT_LONG
		c = getopt_long(argc, argv, optstring, long_options, &i);
#else
		c = getopt (argc, argv, optstring);
#endif
		if (c == -1)
			break;
		if (c == ':') {
			usage (progname);
			return 1;
		}

		switch (c) {
		case 'h': /* help */
			show_help = 1;
			break;
		case 'v': /* version */
			show_version = 1;
			break;
		case 'f':
			if (optarg) {
				config_filename = optarg;
			}
			break;
		default:
			usage(progname);
			return 1;
		}
	}

	if (show_version) {
		printf ("%s version " VERSION "\n", progname);
	}

	if (show_help) {
		usage (progname);
	}

	if (show_version || show_help) {
		return 0;
	}

	if (optind > argc) {
		usage (progname);
		return 1;
	}

        cfg = cfg_read (config_filename);
	if(!cfg){
		printf("Config file error.\n");
		return 1;
	}

	if (optind < argc) {
		dictionary_insert (cfg->dictionary, "Listen", argv[optind]);
	}

        log_open ();

        sighttpd = sighttpd_init (cfg);

	free (cfg);

#ifdef HAVE_OGGZ
	oggstdin_run();
#endif

#ifdef HAVE_SHCODECS
	shrecord_run();
#endif

        signal (SIGINT, sig_handler);
        signal (SIGPIPE, sig_handler);

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
