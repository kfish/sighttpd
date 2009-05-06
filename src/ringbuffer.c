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
//#include <pthread.h>

#include "ringbuffer.h"

void ringbuffer_init(struct ringbuffer *rbuf, void *data, size_t len)
{
        memset (data, 0, len);

	rbuf->pread=rbuf->pwrite=0;
	rbuf->data=data;
	rbuf->size=len;
	rbuf->error=0;

#if 0
        pthread_mutex_init (&rbuf->mutex, NULL);
        pthread_cond_init (&rbuf->cond, NULL);
#endif
}

int ringbuffer_empty(struct ringbuffer *rbuf)
{
	return (rbuf->pread==rbuf->pwrite);
}



ssize_t ringbuffer_free(struct ringbuffer *rbuf)
{
	ssize_t free;

	free = rbuf->pread - rbuf->pwrite;
	if (free <= 0)
		free += rbuf->size;
	return free-1;
}



ssize_t ringbuffer_avail(struct ringbuffer *rbuf)
{
	ssize_t avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}



void ringbuffer_flush(struct ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}

void ringbuffer_reset(struct ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite = 0;
	rbuf->error = 0;
}

void ringbuffer_flush_spinlock_wakeup(struct ringbuffer *rbuf)
{
	unsigned long flags;

        //pthread_mutex_lock (&rbuf->mutex);
	ringbuffer_flush(rbuf);
        //pthread_mutex_unlock (&rbuf->mutex);

        //pthread_cond_signal (&rbuf->cond);
}


ssize_t ringbuffer_writefd(int fd, struct ringbuffer *rbuf)
{
	size_t todo, len;
	size_t split, n;

        todo = len = ringbuffer_avail (rbuf);
	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
                n = write (fd, rbuf->data+rbuf->pread, split);
                if (n == -1) {
                        return -1;
                } else if (n > 0) {
		        todo -= n;
	                rbuf->pread = (rbuf->pread + n) % rbuf->size;
                }
	}

	n = write (fd, rbuf->data+rbuf->pread, todo);
        if (n == -1) {
                return -1;
        } else if (n > 0) {
	        rbuf->pread = (rbuf->pread + n) % rbuf->size;
        }

	return len;
}

ssize_t ringbuffer_readfd(int fd, struct ringbuffer *rbuf)
{
	size_t todo, len;
	size_t split, n;

        todo = len = ringbuffer_free (rbuf);
	split = (rbuf->pwrite + len > rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
                n = read (fd, rbuf->data+rbuf->pwrite, split);
                if (n == -1) {
                        return -1;
                } else if (n > 0) {
		        todo -= n;
	                rbuf->pwrite = (rbuf->pwrite + n) % rbuf->size;
                }
	}

        n = read (fd, rbuf->data+rbuf->pwrite, todo);
        if (n == -1) {
                return -1;
        } else if (n > 0) {
	        rbuf->pwrite = (rbuf->pwrite + n) % rbuf->size;
        }

	return len;
}

ssize_t ringbuffer_read(struct ringbuffer *rbuf, unsigned char *buf, size_t len, int usermem)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;
	if (split > 0) {
		memcpy(buf, rbuf->data+rbuf->pread, split);
		buf += split;
		todo -= split;
		rbuf->pread = 0;
	}
	memcpy(buf, rbuf->data+rbuf->pread, todo);

	rbuf->pread = (rbuf->pread + todo) % rbuf->size;

	return len;
}

ssize_t ringbuffer_write(struct ringbuffer *rbuf, const unsigned char *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	split = (rbuf->pwrite + len > rbuf->size) ? rbuf->size - rbuf->pwrite : 0;

	if (split > 0) {
		memcpy(rbuf->data+rbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		rbuf->pwrite = 0;
	}
	memcpy(rbuf->data+rbuf->pwrite, buf, todo);
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;

	return len;
}
