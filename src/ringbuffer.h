/*
 *
 * based on Linux dvb_ringbuffer.h: ring buffer implementation for the dvb driver
 *
 * Copyright (C) 2003 Oliver Endriss
 * Copyright (C) 2004 Andrew de Quincey
 *
 * based on code originally found in av7110.c & ci.c:
 * Copyright (C) 1999-2003 Ralph Metzler & Marcus Metzler
 *                         for convergence integrated media GmbH
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

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <sys/types.h>

//#include <pthread.h>

struct ringbuffer {
	unsigned char    *data;
	ssize_t           size;
	ssize_t           pread;
	ssize_t           pwrite;
	int               error;

#if 0
        pthread_mutex_t  *mutex;
        pthread_cond_t   *cond;
#endif
};

/*
** Notes:
** ------
** (1) For performance reasons read and write routines don't check buffer sizes
**     and/or number of bytes free/available. This has to be done before these
**     routines are called. For example:
**
**     *** write <buflen> bytes ***
**     free = ringbuffer_free(rbuf);
**     if (free >= buflen)
**         count = ringbuffer_write(rbuf, buffer, buflen);
**     else
**         ...
**
**     *** read min. 1000, max. <bufsize> bytes ***
**     avail = ringbuffer_avail(rbuf);
**     if (avail >= 1000)
**         count = ringbuffer_read(rbuf, buffer, min(avail, bufsize), 0);
**     else
**         ...
**
** (2) If there is exactly one reader and one writer, there is no need
**     to lock read or write operations.
**     Two or more readers must be locked against each other.
**     Flushing the buffer counts as a read operation.
**     Resetting the buffer counts as a read and write operation.
**     Two or more writers must be locked against each other.
*/

/* initialize ring buffer, lock and queue */
extern void ringbuffer_init(struct ringbuffer *rbuf, void *data, size_t len);

/* test whether buffer is empty */
extern int ringbuffer_empty(struct ringbuffer *rbuf);

/* return the number of free bytes in the buffer */
extern ssize_t ringbuffer_free(struct ringbuffer *rbuf);

/* return the number of bytes waiting in the buffer */
extern ssize_t ringbuffer_avail(struct ringbuffer *rbuf);


/*
** Reset the read and write pointers to zero and flush the buffer
** This counts as a read and write operation
*/
extern void ringbuffer_reset(struct ringbuffer *rbuf);


/* read routines & macros */
/* ---------------------- */
/* flush buffer */
extern void ringbuffer_flush(struct ringbuffer *rbuf);

/* flush buffer protected by spinlock and wake-up waiting task(s) */
extern void ringbuffer_flush_spinlock_wakeup(struct ringbuffer *rbuf);

/* peek at byte <offs> in the buffer */
#define RINGBUFFER_PEEK(rbuf,offs)	\
			(rbuf)->data[((rbuf)->pread+(offs))%(rbuf)->size]

/* advance read ptr by <num> bytes */
#define RINGBUFFER_SKIP(rbuf,num)	\
			(rbuf)->pread=((rbuf)->pread+(num))%(rbuf)->size


ssize_t ringbuffer_writefd(int fd, struct ringbuffer *rbuf);
ssize_t ringbuffer_readfd(int, struct ringbuffer *rbuf);

/*
** read <len> bytes from ring buffer into <buf>
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t ringbuffer_read(struct ringbuffer *rbuf, unsigned char *buf,
				   size_t len, int usermem);


/* write routines & macros */
/* ----------------------- */
/* write single byte to ring buffer */
#define RINGBUFFER_WRITE_BYTE(rbuf,byte)	\
			{ (rbuf)->data[(rbuf)->pwrite]=(byte); \
			(rbuf)->pwrite=((rbuf)->pwrite+1)%(rbuf)->size; }
/*
** write <len> bytes to ring buffer
** <usermem> specifies whether <buf> resides in user space
** returns number of bytes transferred or -EFAULT
*/
extern ssize_t ringbuffer_write(struct ringbuffer *rbuf, const unsigned char *buf,
				    size_t len);

#endif /* _RINGBUFFER_H_ */
