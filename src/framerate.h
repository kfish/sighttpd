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
#ifndef __FRAMERATE_H__
#define __FRAMERATE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h> /* Definition of uint64_t */
#include <time.h>

struct framerate {
	int nr_handled;
	int nr_dropped;
	struct timespec start;
	long total_elapsed_us;
	long curr_elapsed_us;
	double fps;
	double prev_fps;

#ifdef HAVE_TIMERFD
	int timer_fd;
#else
	long frame_us;
#endif
};

/* Create a framerate object without timer */
struct framerate * framerate_new_measurer (void);

/* Create a framerate object with timer */
struct framerate * framerate_new_timer (double fps);

/* Destroy a framerate object */
int framerate_destroy (struct framerate * framerate);

/* Mark a frame as done, without waiting. Increments nr_handled */
int framerate_mark (struct framerate * framerate);

/* Wait for the next timeout. Increments nr_handled, and will
 * increment nr_dropped by the number of events missed since the last
 * call to framerate_wait() */
uint64_t framerate_wait (struct framerate * framerate);

/* Time in microseconds since calling framerate_new_*() */
long framerate_elapsed_time (struct framerate * framerate);

/* Mean average FPS over the entire elapsed time */
double framerate_mean_fps (struct framerate * framerate);

/* Instantaneous FPS ... */
double framerate_instantaneous_fps (struct framerate * framerate);

#endif /* __FRAMERATE_H__ */
