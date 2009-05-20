#ifndef __SIGHTTPD_H__
#define __SIGHTTPD_H__

#include "list.h"

struct sighttpd {
	int port;
        list_t * streams;
};

struct sighttpd_child {
        struct sighttpd * sighttpd;
        int accept_fd;
};

struct sighttpd * sighttpd_init (int port);
void sighttpd_close (struct sighttpd * sighttpd);

struct sighttpd_child * sighttpd_child_new (struct sighttpd * sighttpd, int accept_fd);
void sighttpd_child_destroy (struct sighttpd_child * schild);

#endif /* __SIGHTTPD_H__ */
