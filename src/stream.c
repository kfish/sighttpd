#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "params.h"
#include "ringbuffer.h"

/* #define DEBUG */

#define CONTENT_TYPE "video/mpeg4"

static struct ringbuffer stream_rb;
static int active=0;

static void *
stream_writer (void * unused)
{
        size_t available;
        ssize_t n;

        active = 1;

        while (active) {
                n = ringbuffer_readfd (STDIN_FILENO, &stream_rb);

#ifdef DEBUG
                printf ("stream_writer: read %ld bytes\n", n);
#endif
        }

        return NULL;
}

void
stream_init (void)
{
	pthread_t child;
        unsigned char * data;
        size_t len = 4096*16;

        data = malloc (len);

        ringbuffer_init (&stream_rb, data, len);
	pthread_create(&child, 0,  stream_writer, NULL);
	pthread_detach(child);
}

void
stream_close (void)
{
        active = 0;
        free (stream_rb.data);
}

params_t *
stream_append_headers (params_t * response_headers)
{
        char length[16];

        response_headers = params_append (response_headers, "Content-Type", CONTENT_TYPE);
}

int
stream_stream_body (int fd)
{
        size_t available;
        size_t n;

        ringbuffer_flush (&stream_rb);

        while (active) {
#ifdef DEBUG
                        printf ("stream_reader: %ld bytes available\n", available);
#endif
                        n = ringbuffer_writefd (fd, &stream_rb);
                        fsync (fd);
#ifdef DEBUG
                        printf ("stream_reader: wrote %ld bytes to socket\n", n);
#endif
        }
}
