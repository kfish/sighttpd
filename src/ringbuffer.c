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

void ringbuffer_create(struct ringbuffer *buffer, unsigned long order)
{
	char path[] = "/dev/shm/ring-buffer-XXXXXX";
	int file_descriptor;
	void *address;
	int status;

	file_descriptor = mkstemp(path);
	if (file_descriptor < 0)
		report_exceptional_condition();

	status = unlink(path);
	if (status)
		report_exceptional_condition();

	buffer->count_bytes = 1UL << order;
	buffer->write_offset_bytes = 0;
	buffer->read_offset_bytes = 0;

	status = ftruncate(file_descriptor, buffer->count_bytes);
	if (status)
		report_exceptional_condition();

	buffer->address = mmap(NULL, buffer->count_bytes << 1, PROT_NONE,
			       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	if (buffer->address == MAP_FAILED)
		report_exceptional_condition();

	address =
	    mmap(buffer->address, buffer->count_bytes,
		 PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED,
		 file_descriptor, 0);

	if (address != buffer->address)
		report_exceptional_condition();

	address = mmap(buffer->address + buffer->count_bytes,
		       buffer->count_bytes, PROT_READ | PROT_WRITE,
		       MAP_FIXED | MAP_SHARED, file_descriptor, 0);

	if (address != buffer->address + buffer->count_bytes)
		report_exceptional_condition();

	status = close(file_descriptor);
	if (status)
		report_exceptional_condition();
}

void ringbuffer_free(struct ringbuffer *buffer)
{
	int status;

	status = munmap(buffer->address, buffer->count_bytes << 1);
	if (status)
		report_exceptional_condition();
}

void *ringbuffer_write_address(struct ringbuffer *buffer)
{
	return buffer->address + buffer->write_offset_bytes;
						       /*** void pointer arithmetic is a constraint violation. ***/
}

void
ringbuffer_write_advance(struct ringbuffer *buffer,
			  unsigned long count_bytes)
{
	buffer->write_offset_bytes += count_bytes;
}

void *ringbuffer_read_address(struct ringbuffer *buffer)
{
	return buffer->address + buffer->read_offset_bytes;
}

void
ringbuffer_read_advance(struct ringbuffer *buffer,
			 unsigned long count_bytes)
{
	buffer->read_offset_bytes += count_bytes;

	if (buffer->read_offset_bytes >= buffer->count_bytes) {
		buffer->read_offset_bytes -= buffer->count_bytes;
		buffer->write_offset_bytes -= buffer->count_bytes;
	}
}

unsigned long ringbuffer_count_bytes(struct ringbuffer *buffer)
{
	return buffer->write_offset_bytes - buffer->read_offset_bytes;
}

unsigned long ringbuffer_count_free_bytes(struct ringbuffer *buffer)
{
	return buffer->count_bytes - ringbuffer_count_bytes(buffer);
}

void ringbuffer_clear(struct ringbuffer *buffer)
{
	buffer->write_offset_bytes = 0;
	buffer->read_offset_bytes = 0;
}
