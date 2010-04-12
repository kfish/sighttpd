/*
   Copyright (C) 2009 Conrad Parker
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

int shell_stream (int fd, char * cmd)
{
	char buf[4096];
        int total=0;
	FILE *pf;
        size_t ret;

	pf = popen(cmd, "r");
	if (pf == NULL) {
                perror ("popen");
                return 0;
        }

        while ((ret = fread (buf, 1, 4096, pf)) > 0) {
                total += write (fd, buf, ret);
        }

	pclose(pf);

        return total;
}

int shell_copy (char * buf, size_t n, char * cmd)
{
        int total=0;
	FILE *pf;
        size_t ret, remaining=n;

	pf = popen(cmd, "r");
	if (pf == NULL) {
                perror ("popen");
                return 0;
        }

        while (remaining > 0 && !feof(pf) && !ferror(pf)) {
                ret = fread (buf, 1, remaining, pf);
                remaining -= ret;
                buf += ret;
                total += ret;
        }

	pclose(pf);

        return total;
}
