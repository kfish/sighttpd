/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <pthread.h>
#include <oggz/oggz.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"
#include "stream.h"
#include "tempfd.h"

/*#define DEBUG*/

#define DEFAULT_CONTENT_TYPE "application/ogg"

#define x_strdup(s) ((s)?strdup((s)):(NULL))

struct oggstdin {
	const char * path;
	const char * content_type;

	pthread_t main_thread;

	int active;
        struct ringbuffer rb;

	int headers_fd;
	size_t headers_len; /* pointer to current writeable position in headers */
	int nr_headers_got;

	OGGZ * oggz;
};

static struct oggstdin oggstdin_pvt;

static int
oggstdin_read_page (OGGZ * oggz, const ogg_page * og, long serialno, void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;

	if (ogg_page_bos(og)) {
		st->headers_len = 0;
		st->nr_headers_got = 0;
		lseek (st->headers_fd, 0, SEEK_SET);
	}

	if (st->nr_headers_got < 3) {
		st->nr_headers_got += ogg_page_packets (og);
		write (st->headers_fd, og->header, og->header_len);
		st->headers_len += og->header_len;
		write (st->headers_fd, og->body, og->body_len);
		st->headers_len += og->body_len;
		fsync (st->headers_fd);
	} else {
		ringbuffer_write (&st->rb, og->header, og->header_len);
		ringbuffer_write (&st->rb, og->body, og->body_len);
	}

	return (st->active ? OGGZ_CONTINUE : OGGZ_STOP_OK);
}

static void *
oggstdin_main (void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;

	st->oggz = oggz_open_stdio (stdin, OGGZ_READ);
	
	oggz_set_read_page (st->oggz, -1, oggstdin_read_page, st);

	oggz_run (st->oggz);

	oggz_close (st->oggz);

	return NULL;
}

int oggstdin_run (void)
{
	struct oggstdin *st = &oggstdin_pvt;

	return pthread_create (&st->main_thread, NULL, oggstdin_main, st);
}

void
oggstdin_sighandler (void)
{
	struct oggstdin *st = &oggstdin_pvt;

	st->active = 0;

	pthread_join (st->main_thread, NULL);
}

static int
oggstdin_check (http_request * request, void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;

        return !strncmp (request->path, st->path, strlen(st->path));
}

static void
oggstdin_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;
	params_t * r = *response_headers;

        *status_line = http_status_line (HTTP_STATUS_OK);

        r = params_append (r, "Content-Type", st->content_type);
}

static void
oggstdin_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;
        size_t n, avail;
	off_t offset=0;
        int rd;

	while (st->nr_headers_got < 3) {
		usleep (10000);
	}

	if ((n = sendfile (fd, st->headers_fd, &offset, st->headers_len)) == -1) {
		perror ("OggStdin body write");
		return;
	}
	fsync (fd);

        rd = ringbuffer_open (&st->rb);

        while (st->active) {
                while ((avail = ringbuffer_avail (&st->rb, rd)) == 0)
                        usleep (10000);

#ifdef DEBUG
                if (avail != 0) printf ("stream_reader: %ld bytes available\n", avail);
#endif
                n = ringbuffer_writefd (fd, &st->rb, rd);
                if (n == -1) {
                        break;
                }
                
                fsync (fd);
#ifdef DEBUG
                if (n!=0 || avail != 0) printf ("stream_reader: wrote %ld of %ld bytes to socket\n", n, avail);
#endif
        }

        ringbuffer_close (&st->rb, rd);
}

static void
oggstdin_delete (void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;

	free (st->path);
	free (st->content_type);
        free (st->rb.data);

	close (st->headers_fd);

	oggz_close (st->oggz);
}

struct resource *
oggstdin_resource (const char * path, const char * content_type)
{
	struct oggstdin * st = &oggstdin_pvt;
        unsigned char * data, * headers;
        size_t len = 4096*16*32;
	size_t header_len = 10 * 1024;

        if ((data = malloc (len)) == NULL) {
                return NULL;
        }

	if ((st->headers_fd = create_tempfd ("sighttpd-XXXXXXXXXX")) == -1) {
		perror ("create_tempfd");
		return NULL;
	}

	st->path = x_strdup (path);
	if (st->path == NULL) {
		free (data);
		return NULL;
	}
	st->content_type = x_strdup (content_type);
	if (st->content_type == NULL) {
		free (data);
		free (st->path);
		return NULL;
	}

	ringbuffer_init (&st->rb, data, len);
	st->active = 1;

	st->headers_len = 0;
	st->nr_headers_got = 0;

	return resource_new (oggstdin_check, oggstdin_head, oggstdin_body, oggstdin_delete, st);
}

list_t *
oggstdin_resources (Dictionary * config)
{
	list_t * l;
	const char * path;
	const char * ctype;

	l = list_new();

	path = dictionary_lookup (config, "Path");
	ctype = dictionary_lookup (config, "Type");

	if (!ctype) ctype = DEFAULT_CONTENT_TYPE;

	if (path)
		l = list_append (l, oggstdin_resource (path, ctype));

	return l;
}

