/*
 * libshcodecs: A library for controlling SH-Mobile hardware codecs
 * Copyright (C) 2009 Renesas Technology Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

/*
 * This program captures v4l2 input (e.g. from a camera), optionally crops
 * and rotates this, encodes this and shows it on the display.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stropts.h>
#include <stdarg.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <poll.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"

#include "ringbuffer.h"

/* #define DEBUG */

/* Maximum number of cameras handled by the program */
#define MAX_CAMERAS 8

/* Maximum number of encoders per camera */
#define MAX_ENCODERS 8

/* Directory path to the named pipe */
#define TMP_DIR	"/tmp"

#define x_strdup(s) ((s)?strdup((s)):(NULL))

struct encode_data {
	pthread_t thread;
	int alive;

	char path[MAXPATHLEN];
	char ctrl_filename[MAXPATHLEN];
	char fifo[MAXPATHLEN];
	char fifo_path[MAXPATHLEN];

	struct ringbuffer rb;
};

struct private_data {
	pthread_t main_thread;

	int nr_encoders;
	struct encode_data encdata[MAX_ENCODERS];

	struct pollfd pfds[MAX_ENCODERS];
	pid_t shrecord_pid;
	pid_t pid;

	int do_preview;
};

void debug_printf(const char *fmt, ...)
{
#ifdef DEBUG
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
#endif
}

struct private_data pvt_data;

static int alive=1;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************/

void
shrecord_cleanup (void)
{
	struct private_data *pvt = &pvt_data;
	struct encode_data *eds = pvt->encdata;
	int i, status;

	/* kill child process if needed */
	if (pvt->shrecord_pid) {
		kill(pvt->shrecord_pid, SIGKILL);
		waitpid(pvt->shrecord_pid, &status, 0);
	}

	/* clean files & fifos */
	for(i = 0; i < pvt->nr_encoders; i++) {
		unlink(eds[i].fifo_path);
		unlink(eds[i].ctrl_filename);
	}
}

void
shrecord_sighandler (void)
{
	struct private_data *pvt = &pvt_data;

	shrecord_cleanup();
}

void
shrecord_sigchld_handler (int ignore)
{
	struct private_data *pvt = &pvt_data;
	int status;

	waitpid(pvt->shrecord_pid, &status, 0);
	pvt->shrecord_pid = 0;

	shrecord_cleanup();
}
#define BUFFER_SIZE	(128 * 1024)

void * shrecord_main (void * data)
{
	struct private_data *pvt = (struct private_data *)data;
	struct pollfd *pfds = pvt->pfds;
	struct encode_data *eds = pvt->encdata;
	int i, n, count;
	unsigned char buffer[BUFFER_SIZE];
	char *argv[MAX_ENCODERS + 3];
	static char *shcodec_record = "shcodecs-record";
	static char *preview_off = "-P";

	/* structure argument */
	n = 0;
	argv[n++] = shcodec_record;
	if (!pvt->do_preview)
		argv[n++] = preview_off;
	for(i = 0; i < pvt->nr_encoders; i++)
	    	argv[n++] = eds[i].ctrl_filename;

	/* launch shcodec_record */
	pvt->shrecord_pid = fork();
	if (pvt->shrecord_pid < 0) {
		fprintf(stderr, "Can't fork()\n");
		goto clean;
	} if (pvt->shrecord_pid == 0) {
		execvp(argv[0], argv);

		perror("execvp() failed");
		exit(1);
	} else {
	    	fprintf(stderr, "Launched shcodec-record successively\n");
		signal(SIGCHLD, shrecord_sigchld_handler);
	}

	fprintf(stderr, "# of encs = %d\n", pvt->nr_encoders);

	if (pvt->nr_encoders == 0)
		return NULL;

	for(i = 0; i < pvt->nr_encoders; i++)
		pfds[i].fd = -1;

	for(i = 0; i < pvt->nr_encoders; i++) {
		fprintf(stderr, "fifo for #%d = '%s'\n",i, eds[i].fifo_path);
		pfds[i].fd = open(eds[i].fifo_path, O_RDWR, 0);
		if (pfds[i].fd < 0) {
			fprintf(stderr, "Can't open fifo - %s\n", eds[i].fifo_path);
			goto clean;
		}	
		pfds[i].events = POLLIN;
		pfds[i].revents = 0;
	}

	while(1) {
		n = poll(pfds, pvt->nr_encoders, 0);

		if (n < 0) {
			fprintf(stderr, "poll() failed.\n");
			goto clean;
		}

		if (n > 0) {
			for(i = 0; i < pvt->nr_encoders; i++) {
				if (pfds[i].revents & POLLIN) {
					pfds[i].revents = 0;

					count = read(pfds[i].fd, buffer, BUFFER_SIZE);
					if (count < 0) {
						fprintf(stderr, "read() failed on %s.\n",
							eds[i].fifo);
						goto clean;
					}
					ringbuffer_write(&eds[i].rb, buffer, count);
				}
			}
		}

	}
	

clean:
	shrecord_cleanup();

	/* shrecord_run() and caller anyway ignore the return value... */
	return NULL;
}

int shrecord_run (void)
{
	struct private_data *pvt = &pvt_data;

	fprintf(stderr, "starting shrecord_main thread\n");

	return pthread_create(&pvt->main_thread, NULL, shrecord_main, pvt);
}

static int
shrecord_check (http_request * request, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;

        return !strncmp (request->path, ed->path, strlen(ed->path));
}

static void
shrecord_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;
	params_t * r = *response_headers;
        char length[16];

        *status_line = http_status_line (HTTP_STATUS_OK);

        r = params_append (r, "Content-Type", "video/mp4");
}

static void
shrecord_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;
        size_t n, avail;
        int rd;

        rd = ringbuffer_open (&ed->rb);

        while (ed->alive) {
                while ((avail = ringbuffer_avail (&ed->rb, rd)) == 0)
                        usleep (10000);

#ifdef DEBUG
                if (avail != 0) printf ("%s: %ld bytes available\n", __func__, avail);
#endif
                n = ringbuffer_writefd (fd, &ed->rb, rd);
                if (n == -1) {
                        break;
                }

                fsync (fd);
#ifdef DEBUG
                if (n!=0 || avail != 0) printf ("%s: wrote %ld of %ld bytes to socket\n", __func__, n, avail);
#endif
        }

        ringbuffer_close (&ed->rb, rd);
}

static void
shrecord_delete (void * data)
{
	struct encode_data * ed = (struct encode_data *)data;

        free (ed->rb.data);
}

static int
shrecord_tweak_ctrlfile(const char *orig_ctrl, const char *new_ctrl, const char *fifo)
{
	char path[MAXPATHLEN];
	char buffer[BUFSIZ];
	FILE *ofp, *nfp;

	if ((nfp = fopen(new_ctrl, "w")) == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", new_ctrl);
		return -1;
	}

	if ((ofp = fopen(orig_ctrl, "r")) == NULL) {
		fprintf(stderr, "Can't open original control file '%s'\n", orig_ctrl);
		return -1;
	}

	while(fgets(buffer, BUFSIZ, ofp)) {
		if (!strncmp(buffer, "output_directry", 15))
			fprintf(nfp, "output_directry = %s;\n", TMP_DIR);
		else if (!strncmp(buffer, "output_stream_file", 18))
			fprintf(nfp, "output_stream_file = %s;\n", fifo);
		else
			fputs(buffer, nfp);
	}

	fclose(nfp);
	fclose(ofp);

	return 0;
}

static int
shrecord_mkfifo ( char *fifo ) 
{
	struct stat stat;

	if (!lstat(fifo, &stat))
		return (stat.st_mode & S_IFIFO) ? 0 : -1;

	return mkfifo(fifo, 077);
}

struct resource *
shrecord_resource (const char * path, const char * ctlfile)
{
	struct encode_data * ed = NULL;
	struct private_data *pvt = &pvt_data;
	int return_code;
        unsigned char * data = NULL;
        size_t len = 4096*16*32;

	if (pvt->nr_encoders > MAX_ENCODERS)
		return NULL;

	ed = &pvt->encdata[pvt->nr_encoders];

	strncpy(ed->path, path, MAXPATHLEN);
	snprintf(ed->ctrl_filename, MAXPATHLEN, TMP_DIR "/shrecord_ctrl-%d.%d",
		 pvt->pid, pvt->nr_encoders);
	snprintf(ed->fifo, MAXPATHLEN, "shrecord_fifo-%d.%d",
		 pvt->pid, pvt->nr_encoders);
	snprintf(ed->fifo_path, MAXPATHLEN, TMP_DIR "/%s", ed->fifo);

	/* create tweaked control file */
	if (shrecord_tweak_ctrlfile(ctlfile, ed->ctrl_filename, ed->fifo) < 0)
		return NULL;

	/* create fifo */
	if (shrecord_mkfifo(ed->fifo_path) < 0) 
		return NULL;

	/* init ring buffer */
        if ((data = malloc (len)) == NULL)
		return NULL;
        ringbuffer_init (&ed->rb, data, len);

	ed->alive = 1;
	pvt->nr_encoders++;

	return resource_new (shrecord_check, shrecord_head, shrecord_body, shrecord_delete, ed);
}

list_t *
shrecord_resources (Dictionary * config)
{
	struct private_data *pvt = &pvt_data;
	list_t * l;
	const char * path;
	const char * ctlfile;
	const char * preview;
	struct resource * r;

	l = list_new();

	pvt->pid = getpid();

	path = dictionary_lookup (config, "Path");
	ctlfile = dictionary_lookup (config, "CtlFile");

	if (path && ctlfile) {
		if ((r = shrecord_resource (path, ctlfile)) != NULL)
			l = list_append (l, r);
	}

	if ((preview = dictionary_lookup (config, "Preview")) != NULL) {
		if (!strncasecmp (preview, "on", 2))
			pvt->do_preview=1;
		else if (!strncasecmp (preview, "off", 3))
			pvt->do_preview=0;
	}

	return l;
}

int
shrecord_init (void)
{
	struct private_data *pvt = &pvt_data;
	int ret;

	memset (pvt, 0, sizeof (pvt_data));

	/* Set preview on by default, allow to turn off by setting "Preview off" */
	pvt->do_preview = 1;

	return 0;
}
