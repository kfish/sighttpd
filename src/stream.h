#ifndef __STREAM_H__
#define __STREAM_H__

#include "params.h"
#include "ringbuffer.h"

struct stream {
        int input_fd;
        int active;
        struct ringbuffer rb;
};

struct stream * stream_open (int fd);
void stream_close (struct stream * stream);
params_t * stream_append_headers (params_t * response_headers, struct stream * stream);
int stream_stream_body (int fd, struct stream * stream);

#endif /* __STREAM_H__ */
