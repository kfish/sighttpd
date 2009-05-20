#include "sighttpd.h"
#include "stream.h"
#include "list.h"

struct sighttpd * sighttpd_init (int port)
{
        struct sighttpd * sighttpd;

        if ((sighttpd = malloc (sizeof(*sighttpd))) == NULL)
                return NULL;

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
