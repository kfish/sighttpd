#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <netdb.h>

#include "dictionary.h"
#include "sighttpd.h"
#include "stream.h"
#include "list.h"

struct sighttpd * sighttpd_init (Dictionary * config)
{
        struct sighttpd * sighttpd;
        const char *portname;
        int port;

        if ((sighttpd = malloc (sizeof(*sighttpd))) == NULL)
                return NULL;

        portname = dictionary_lookup (config, "Listen");
        if (portname == NULL) {
                fprintf (stderr, "Portname not specified.\n");
                exit (1);
        }

        /* Get server's IP and standard service connection* */
        if (!isdigit(portname[0])) {
                struct servent *srv = getservbyname(portname, "tcp");

                if (srv == NULL) {
                        perror(portname);
                        exit (1);
                }

                printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
                port = srv->s_port;
        } else {
                port = htons(atoi(portname));
        }

        sighttpd->port = port;
        sighttpd->streams = list_new();

        return sighttpd;
}

void sighttpd_close (struct sighttpd * sighttpd)
{
        list_free_with (sighttpd->streams, stream_close);
        free (sighttpd);
}

struct sighttpd_child *
sighttpd_child_new (struct sighttpd * sighttpd, int accept_fd)
{
        struct sighttpd_child * schild;

        if ((schild = malloc (sizeof(*schild))) == NULL)
                return NULL;

        schild->sighttpd = sighttpd;
        schild->accept_fd = accept_fd;

        return schild;
}

void
sighttpd_child_destroy (struct sighttpd_child * schild)
{
        close (schild->accept_fd);
        free (schild);
}
