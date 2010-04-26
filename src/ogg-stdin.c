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

#define DEFAULT_CONTENT_TYPE "application/ogg"

#define x_strdup(s) ((s)?strdup((s)):(NULL))

struct oggstdin {
	const char * path;
	const char * content_type;
        struct stream * stream;
};

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
oggstdin_delete (void * data)
{
	struct oggstdin * st = (struct oggstdin *)data;

	stream_close (st->stream);

	free (st->path);
	free (st->content_type);

	free (st);
}

struct resource *
oggstdin_resource (const char * path, int fd, const char * content_type)
{
	struct oggstdin * st;

	if ((st = calloc (1, sizeof(*st))) == NULL)
		return NULL;

	st->path = x_strdup (path);
	st->content_type = x_strdup (content_type);
	st->stream = stream_open (fd);

	return resource_new (oggstdin_check, oggstdin_head, oggstdin_body, oggstdin_delete, st);
}

struct resource *
oggstdin_resource_open (const char * urlpath, const char * filepath, const char * content_type)
{
        int fd;

        if ((fd = open (filepath, O_RDONLY)) == -1)
		return NULL;

	return oggstdin_resource (urlpath, fd, content_type);
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
		l = list_append (l, oggstdin_resource (path, STDIN_FILENO, ctype));

	return l;
}

