/*
 * libshcodecs: A library for controlling SH-Mobile hardware codecs
 * Copyright (C) 2009 Renesas Technology Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>		/* Definition of uint64_t */

#include "framerate.h"

#define handle_error(msg) \
	       do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define U_SEC_PER_SEC 1000000
#define N_SEC_PER_SEC 1000000000

struct framerate * framerate_new_measurer (void)
{
	struct framerate * framerate;
	struct timespec * now;

	framerate = calloc (1, sizeof(*framerate));
	if (framerate == NULL)
		return NULL;

	now = &framerate->start;

	if (clock_gettime(CLOCK_MONOTONIC, now) == -1)
		goto err_out;

	return framerate;

err_out:
	free (framerate);
	return NULL;
}

struct framerate * framerate_new_timer (double fps)
{
	struct framerate * framerate;
	struct itimerspec new_value;
	struct timespec * now;
	long interval;

	if (fps < 0.0)
		return NULL;

	framerate = framerate_new_measurer ();
	if (framerate == NULL)
		return NULL;

	/* Treat fps == 0.0 as a measurer, and avoid divide-by-zero */
	if (fps == 0.0)
		return framerate;

	interval = (long) (N_SEC_PER_SEC / fps);

	now = &framerate->start;

#ifdef HAVE_TIMERFD
	/* Create a CLOCK_REALTIME absolute timer with initial
	 * expiration "now" and interval for the given framerate */

	new_value.it_value.tv_sec = now->tv_sec;
	new_value.it_value.tv_nsec = now->tv_nsec;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = interval;

	framerate->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (framerate->timer_fd == -1)
		goto err_out;

	if (timerfd_settime(framerate->timer_fd, TFD_TIMER_ABSTIME,
				&new_value, NULL) == -1)
		goto err_out;
#else
	framerate->frame_us = interval / 1000;
#endif

	return framerate;

err_out:
	free (framerate);
	return NULL;
}

int framerate_destroy (struct framerate * framerate)
{
	int ret=0;

	if (framerate == NULL) return -1;

#ifdef HAVE_TIMERFD
	if (framerate->timer_fd != 0)
	    ret = close (framerate->timer_fd);
#endif

	free (framerate);

	return ret;
}

long framerate_elapsed_time (struct framerate * framerate)
{
	if (framerate == NULL) return -1;

	return framerate->total_elapsed_us;
}

double framerate_mean_fps (struct framerate * framerate)
{
	if (framerate == NULL) return -1.0;

	return (double)framerate->nr_handled * U_SEC_PER_SEC /
					framerate->total_elapsed_us;
}

double framerate_instantaneous_fps (struct framerate * framerate)
{
	double curr_fps;

	if (framerate == NULL) return -1.0;

	if (framerate->curr_elapsed_us == 0)
		return 0.0;

	if (framerate->curr_elapsed_us > 1000) {
		curr_fps = (double)U_SEC_PER_SEC/framerate->curr_elapsed_us;
	} else {
		curr_fps = framerate->prev_fps;
	}

	framerate->prev_fps = curr_fps;

	return curr_fps;
}

/* Total microseconds elapsed */
static long
framerate_elapsed_us (struct framerate * framerate)
{
	struct timespec curr;
	long secs, nsecs;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &curr);
	if (ret == -1) return ret;

	secs = curr.tv_sec - framerate->start.tv_sec;
	nsecs = curr.tv_nsec - framerate->start.tv_nsec;
	if (nsecs < 0) {
		secs--;
		nsecs += N_SEC_PER_SEC;
	}

	return (secs*U_SEC_PER_SEC) + nsecs/1000;
}

int framerate_mark (struct framerate * framerate)
{
	long prev_elapsed_us;

	if (framerate == NULL) return -1;

	framerate->nr_handled++;

	prev_elapsed_us = framerate->total_elapsed_us;

	framerate->total_elapsed_us = framerate_elapsed_us (framerate);
	framerate->curr_elapsed_us = framerate->total_elapsed_us - prev_elapsed_us;

	return 0;
}

uint64_t
framerate_wait (struct framerate * framerate)
{
	uint64_t exp;
#ifdef HAVE_TIMERFD
	ssize_t s;

	if (framerate == NULL) return 0;

	s = read(framerate->timer_fd, &exp, sizeof(uint64_t));
	if (s != sizeof(uint64_t))
		handle_error("read");

#else
	long expected, delta;

	if (framerate == NULL) return 0;

	expected = framerate->total_elapsed_us + framerate->frame_us;
	delta = expected - framerate_elapsed_us (framerate);

	if (delta > 0) {
		usleep (delta);
		exp = 1;
	} else {
		exp = 1 + (-delta) / framerate->frame_us;
	}

#endif
	framerate_mark (framerate);
	framerate->nr_dropped += exp-1;

	return exp;
}
