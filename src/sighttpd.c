#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <netdb.h>

#include "cfg-read.h"
#include "dictionary.h"
#include "sighttpd.h"
#include "resource.h"
#include "stream.h"
#include "list.h"

#include "status.h"
#include "fdstream.h"
#include "flim.h"
#include "uiomux.h"
#include "kongou.h"
#include "statictext.h"

struct sighttpd * sighttpd_init (struct cfg * cfg)
{
        struct sighttpd * sighttpd;
        const char *portname;
        int port;

        if ((sighttpd = malloc (sizeof(*sighttpd))) == NULL)
                return NULL;

        portname = dictionary_lookup (cfg->dictionary, "Listen");
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

                port = ntohs(srv->s_port);
        } else {
                port = atoi(portname);
        }

        sighttpd->port = port;

	sighttpd->resources = cfg->resources;

	sighttpd->resources = list_append (sighttpd->resources, status_resource(sighttpd));
	sighttpd->resources = list_join (sighttpd->resources, fdstream_resources(cfg->dictionary));
	sighttpd->resources = list_append (sighttpd->resources, flim_resource());
	sighttpd->resources = list_append (sighttpd->resources, uiomux_resource());
	sighttpd->resources = list_append (sighttpd->resources, kongou_resource());
	sighttpd->resources = list_join (sighttpd->resources, statictext_resources (cfg->dictionary));

        return sighttpd;
}

void sighttpd_close (struct sighttpd * sighttpd)
{
	list_free_with (sighttpd->resources, resource_delete);
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
