#ifndef __STREAM_H__
#define __STREAM_H__

void stream_init (void);
void stream_close (void);
params_t * stream_append_headers (params_t * response_headers);
int stream_stream_body (FILE * stream);

#endif /* __STREAM_H__ */
