/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> /* STDIN_FILENO */

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"
#include "stream.h"

#define DEFAULT_CONTENT_TYPE "video/mp4"

#define x_strdup(s) ((s)?strdup((s)):(NULL))

struct fdstream {
	const char * path;
	const char * content_type;
        struct stream * stream;
};

static int
fdstream_check (http_request * request, void * data)
{
	struct fdstream * st = (struct fdstream *)data;

        return !strncmp (request->path, st->path, strlen(st->path));
}

static void
fdstream_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
	struct fdstream * st = (struct fdstream *)data;
	params_t * r = *response_headers;

        *status_line = http_status_line (HTTP_STATUS_OK);

        r = params_append (r, "Content-Type", (char *)st->content_type);
}

static void
fdstream_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct fdstream * st = (struct fdstream *)data;
	struct stream * stream = st->stream;
        size_t n, avail;
        int rd;

        rd = ringbuffer_open (&stream->rb);

        while (stream->active) {
                while ((avail = ringbuffer_avail (&stream->rb, rd)) == 0)
                        usleep (10000);

#ifdef DEBUG
                if (avail != 0) printf ("stream_reader: %ld bytes available\n", avail);
#endif
                n = ringbuffer_writefd (fd, &stream->rb, rd);
                if (n == -1) {
                        break;
                }
                
                fsync (fd);
#ifdef DEBUG
                if (n!=0 || avail != 0) printf ("stream_reader: wrote %ld of %ld bytes to socket\n", n, avail);
#endif
        }

        ringbuffer_close (&stream->rb, rd);
}

static void
fdstream_delete (void * data)
{
	struct fdstream * st = (struct fdstream *)data;

	stream_close (st->stream);

	free ((char *)st->path);
	free ((char *)st->content_type);

	free (st);
}

struct resource *
fdstream_resource (const char * path, int fd, const char * content_type)
{
	struct fdstream * st;

	if ((st = calloc (1, sizeof(*st))) == NULL)
		return NULL;

	st->path = x_strdup (path);
	if (st->path == NULL) {
		free (st);
		return NULL;
	}

	st->content_type = x_strdup (content_type);
	if (st->content_type == NULL) {
		free (st);
		free ((char *)st->path);
		return NULL;
	}

	st->stream = stream_open (fd);
	if (st->stream == NULL) {
		free (st);
		free ((char *)st->path);
		free ((char *)st->content_type);
		return NULL;
	}

	return resource_new (fdstream_check, fdstream_head, fdstream_body, fdstream_delete, st);
}

struct resource *
fdstream_resource_open (const char * urlpath, const char * filepath, const char * content_type)
{
        int fd;

        if ((fd = open (filepath, O_RDONLY)) == -1)
		return NULL;

	return fdstream_resource (urlpath, fd, content_type);
}

list_t *
fdstream_resources (Dictionary * config)
{
	list_t * l;
	const char * path;
	const char * ctype;
	struct resource * r;

	l = list_new();

	path = dictionary_lookup (config, "Path");
	ctype = dictionary_lookup (config, "Type");

	if (!ctype) ctype = DEFAULT_CONTENT_TYPE;

	if (path) {
		if ((r = fdstream_resource (path, STDIN_FILENO, ctype)) != NULL)
			l = list_append (l, r);
	}

	/* fdstream_resource_open ("/stream2", "/tmp/stream2.264", "video/mp4"); */

	return l;
}

