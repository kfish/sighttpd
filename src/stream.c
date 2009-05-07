#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "params.h"
#include "ringbuffer.h"

/* #define DEBUG */

#define CONTENT_TYPE "video/mp4"

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
                if (n!=0) printf ("stream_writer: read %ld bytes\n", n);
#endif
        }

        return NULL;
}

void
stream_init (void)
{
	pthread_t child;
        unsigned char * data;
        size_t len = 4096*16*32;

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
        size_t n;
        int rd;

        rd = ringbuffer_open (&stream_rb);

        while (active) {
                        n = ringbuffer_writefd (fd, &stream_rb, rd);
                        if (n == -1) {
                                break;
                        }
                        
                        fsync (fd);
#ifdef DEBUG
                        if (n!=0) printf ("stream_reader: wrote %ld bytes to socket\n", n);
#endif
        }

        ringbuffer_close (&stream_rb, rd);

        return 0;
}
