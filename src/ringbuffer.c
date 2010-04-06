/*
 *
 * based on Linux dvb_ringbuffer.c: ring buffer implementation for the dvb driver
 *
 * Copyright (C) 2003 Oliver Endriss
 * Copyright (C) 2004 Andrew de Quincey
 *
 * based on code originally found in av7110.c & ci.c:
 * Copyright (C) 1999-2003 Ralph  Metzler
 *                       & Marcus Metzler for convergence integrated media GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <pthread.h>

#include "ringbuffer.h"

#define RDOPEN(rbuf,rd) (rbuf->readers & (1<<rd))

ssize_t ringbuffer_avail(struct ringbuffer *rbuf, int readd)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread[readd];
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

static void ringbuffer_update_min(struct ringbuffer *rbuf)
{
	int i;
	ssize_t avail, min_avail = -1, min_pread;

        if (rbuf->readers == 0)
                return;

	pthread_mutex_lock(&rbuf->mutex);
	for (i = 0; i < MAX_READERS; i++) {
		if (RDOPEN(rbuf, i)) {
			avail = ringbuffer_avail(rbuf, i);
			if (avail > min_avail) {
				min_pread = rbuf->pread[i];
			}
		}
	}
	rbuf->min_pread = min_pread;
	pthread_mutex_unlock(&rbuf->mutex);
}

void ringbuffer_reset(struct ringbuffer *rbuf)
{
	int i;

	for (i = 0; i < MAX_READERS; i++) {
		rbuf->pread[i] = 0;
	}
        rbuf->pwrite = 0;

	pthread_mutex_lock(&rbuf->mutex);
	rbuf->min_pread = 0;
	pthread_mutex_unlock(&rbuf->mutex);
}

void ringbuffer_init(struct ringbuffer *rbuf, void *data, size_t len)
{
	int i;

	memset(data, 0, len);

	rbuf->readers = 0;
	rbuf->data = data;
	rbuf->size = len;

	pthread_mutex_init(&rbuf->mutex, NULL);
#if 0
	pthread_cond_init(&rbuf->cond, NULL);
#endif

	ringbuffer_reset(rbuf);

}

/* Returns a read descriptor */
int ringbuffer_open(struct ringbuffer *rbuf)
{
	int i, r;

	for (i = 0; i < MAX_READERS; i++) {
		r = 1 << i;
		if (!(rbuf->readers & r)) {
			rbuf->readers |= r;
			rbuf->pread[i] = rbuf->pwrite;
			return i;
		}
	}

	return -1;
}

/* Close a read descriptor */
void ringbuffer_close(struct ringbuffer *rbuf, int readd)
{
	if (readd >= MAX_READERS)
		return;

	rbuf->readers ^= (1 << readd);
	ringbuffer_update_min(rbuf);

	return;
}


int ringbuffer_empty(struct ringbuffer *rbuf, int readd)
{
	return (rbuf->pread[readd] == rbuf->pwrite);
}



ssize_t ringbuffer_free(struct ringbuffer * rbuf)
{
	ssize_t free;

        if (rbuf->readers == 0) {
                free = rbuf->size;
        } else {
	        free = rbuf->min_pread - rbuf->pwrite;
	        if (free <= 0)
		free += rbuf->size;
        }
	return free - 1;
}

void ringbuffer_flush(struct ringbuffer *rbuf, int readd)
{
	rbuf->pread[readd] = rbuf->pwrite;
	ringbuffer_update_min(rbuf);
}

ssize_t ringbuffer_writefd(int fd, struct ringbuffer *rbuf, int readd)
{
	size_t todo, len;
	size_t split, n, nwritten=0;

	todo = len = ringbuffer_avail(rbuf, readd);
        if (len == 0)
                return 0;

	split =
	    (rbuf->pread[readd] + len >
	     rbuf->size) ? rbuf->size - rbuf->pread[readd] : 0;
	if (split > 0) {
		n = write(fd, rbuf->data + rbuf->pread[readd], split);
		if (n == -1) {
			return -1;
		} else if (n > 0) {
			todo -= n;
                        nwritten += n;
			rbuf->pread[readd] =
			    (rbuf->pread[readd] + n) % rbuf->size;
		}
	}

        if (split == 0 || nwritten == split) {
		n = write(fd, rbuf->data + rbuf->pread[readd], todo);
		if (n == -1) {
			return -1;
		} else if (n > 0) {
               		nwritten += n;
			rbuf->pread[readd] = (rbuf->pread[readd] + n) % rbuf->size;
		}
        }

	ringbuffer_update_min(rbuf);

	return nwritten;
}

ssize_t ringbuffer_readfd(int fd, struct ringbuffer * rbuf)
{
	size_t todo, len;
	size_t split, n, nread=0;

	todo = len = ringbuffer_free(rbuf);
	split =
	    (rbuf->pwrite + len >
	     rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
		n = read(fd, rbuf->data + rbuf->pwrite, split);
		if (n == -1) {
			return -1;
		} else if (n > 0) {
			todo -= n;
                        nread = n;
			rbuf->pwrite = (rbuf->pwrite + n) % rbuf->size;
		}
	}

	n = read(fd, rbuf->data + rbuf->pwrite, todo);
	if (n == -1) {
		return -1;
	} else if (n > 0) {
                nread += n;
		rbuf->pwrite = (rbuf->pwrite + n) % rbuf->size;
	}

	return nread;
}

ssize_t ringbuffer_read(struct ringbuffer * rbuf, int readd,
			unsigned char *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split =
	    (rbuf->pread[readd] + len >
	     rbuf->size) ? rbuf->size - rbuf->pread[readd] : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data + rbuf->pread[readd], split);
		buf += split;
		todo -= split;
		rbuf->pread[readd] = 0;
	}
	memcpy(buf, rbuf->data + rbuf->pread[readd], todo);

	rbuf->pread[readd] = (rbuf->pread[readd] + todo) % rbuf->size;

	ringbuffer_update_min(rbuf);

	return len;
}

ssize_t ringbuffer_write(struct ringbuffer * rbuf,
			 const unsigned char *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split =
	    (rbuf->pwrite + len >
	     rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
		memcpy(rbuf->data + rbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		rbuf->pwrite = 0;
	}
	memcpy(rbuf->data + rbuf->pwrite, buf, todo);
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;

	return len;
}
