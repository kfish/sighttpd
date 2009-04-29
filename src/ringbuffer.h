#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#define report_exceptional_condition() abort ()

struct ringbuffer {
	void *address;

	unsigned long count_bytes;
	unsigned long write_offset_bytes;
	unsigned long read_offset_bytes;
};

void ringbuffer_create(struct ringbuffer *buffer, unsigned long order);
void ringbuffer_free(struct ringbuffer *buffer);
void *ringbuffer_write_address(struct ringbuffer *buffer);
void ringbuffer_write_advance(struct ringbuffer *buffer,
                               unsigned long count_bytes);
void *ringbuffer_read_address(struct ringbuffer *buffer);
void ringbuffer_read_advance(struct ringbuffer *buffer,
			 unsigned long count_bytes);
unsigned long ringbuffer_count_bytes(struct ringbuffer *buffer);
unsigned long ringbuffer_count_free_bytes(struct ringbuffer *buffer);
void ringbuffer_clear(struct ringbuffer *buffer);

#endif /* __RINGBUFFER_H__ */
