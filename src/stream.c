#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "params.h"
#include "ringbuffer.h"

/* #define DEBUG */

static struct ringbuffer stream_rb;
static int active=0;

static void *
stream_writer (void * unused)
{
        size_t available;
        ssize_t n;

        active = 1;

        while (active) {
                available = ringbuffer_count_free_bytes (&stream_rb);
#ifdef DEBUG
                printf ("stream_writer: %ld bytes available\n", available);
#endif
                n = read (STDIN_FILENO, ringbuffer_write_address(&stream_rb), available);
#ifdef DEBUG
                printf ("stream_writer: read %ld bytes\n", n);
#endif
                ringbuffer_write_advance (&stream_rb, n);
        }

        return NULL;
}

void
stream_init (void)
{
	pthread_t child;

        ringbuffer_create (&stream_rb, 16);
	pthread_create(&child, 0,  stream_writer, NULL);
	pthread_detach(child);
}

void
stream_close (void)
{
        active = 0;
        ringbuffer_free (&stream_rb);
}

params_t *
stream_append_headers (params_t * response_headers)
{
        char length[16];

        response_headers = params_append (response_headers, "Content-Type", "text/plain");
}

int
stream_stream_body (FILE * stream)
{
        size_t available;
        size_t n;

        available = ringbuffer_count_bytes (&stream_rb);
        ringbuffer_read_advance (&stream_rb, available-1);

        while (active) {
                available = ringbuffer_count_bytes (&stream_rb);
                if (available > 0) {
#ifdef DEBUG
                        printf ("stream_reader: %ld bytes available\n", available);
#endif
                        n = fwrite (ringbuffer_read_address (&stream_rb), 1, available, stream);
#ifdef DEBUG
                        printf ("stream_reader: wrote %ld bytes\n", n);
#endif
                        ringbuffer_read_advance (&stream_rb, n);
                        fflush (stream);
                }
        }
}
