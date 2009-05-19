
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
		/* Handle error */ ;
                perror ("popen");
        }

        while ((ret = fread (buf, 1, 4096, pf)) > 0) {
                total += write (fd, buf, ret);
        }

	pclose(pf);

        return total;
}
