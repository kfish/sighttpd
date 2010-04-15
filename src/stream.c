/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "stream.h"
#include "params.h"
#include "ringbuffer.h"

/* #define DEBUG */

static void *
stream_writer (struct stream * stream)
{
        size_t available;
        ssize_t n;
        fd_set rfds;
        struct timeval tv;
        int retval;

        stream->active = 1;

        while (stream->active) {
                FD_ZERO (&rfds);
                FD_SET (stream->input_fd, &rfds);

                tv.tv_sec = 5;
                tv.tv_usec = 0;
                retval = select (1, &rfds, NULL, NULL, &tv);
                if (retval == -1)
                        perror ("select");
                else if (retval) {
                        n = ringbuffer_readfd (stream->input_fd, &stream->rb);
#ifdef DEBUG
                        if (n!=0) printf ("stream_writer: read %ld bytes\n", n);
#endif
                }
        }

        return NULL;
}

struct stream *
stream_open (int fd)
{
        struct stream * stream;
	pthread_t child;
        unsigned char * data;
        size_t len = 4096*16*32;

        if ((stream = malloc (sizeof(*stream))) == NULL)
                return NULL;

        if ((data = malloc (len)) == NULL) {
                free (stream);
                return NULL;
        }

	stream->input_fd = fd;

        ringbuffer_init (&stream->rb, data, len);
	pthread_create(&child, 0, stream_writer, stream);
	pthread_detach(child);

        return stream;
}

void
stream_close (struct stream * stream)
{
        stream->active = 0;
        free (stream->rb.data);

        free (stream);
}
