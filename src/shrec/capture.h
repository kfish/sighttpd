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

#ifndef __CAPTURE_H__
#define __CAPTURE_H__

struct capture_;
typedef struct capture_ capture;

typedef void (*capture_callback) (capture * cap, const unsigned char *frame_data,
				     size_t length, void *user_data);

capture *capture_open(const char *device_name, int width, int height);

capture *capture_open_userio(const char *device_name, int width, int height);

void capture_close(capture * cap);

/**
 * Set the data output mode to use physical addresses.
 * If the calling application is interfacing to other IP blocks such as
 * the VEU, then set this function. Otherwise, captured frame output will
 * be mapped to userspace addresses, usable by normal applications.
 * \param cap The capture handle
 * \param use_physical Flag: Physical addresses will be reported for
 * output frame data if set to a non-zero value.
 * \retval 0 Success
 */
int
capture_set_use_physical(capture * cap, int use_physical);

void capture_start_capturing(capture * cap);

void capture_stop_capturing(capture * cap);

void capture_get_frame(capture * cap, capture_callback cb,
			  void *user_data);

void capture_queue_buffer(capture * cap, const void * buffer_data);

/* Get the properties of the captured frames 
 * The v4l device may not support the request size
 */
int capture_get_width(capture * cap);
int capture_get_height(capture * cap);
unsigned int capture_get_pixel_format(capture * cap);

#endif				/* __CAPTURE_H__ */
