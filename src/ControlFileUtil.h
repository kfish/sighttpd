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

#ifndef	CONTROL_FILE_UTIL_H
#define	CONTROL_FILE_UTIL_H

#include <shcodecs/shcodecs_encoder.h>
#include "avcbencsmp.h"

int ctrlfile_get_params(const char *ctrl_file,
		    APPLI_INFO * appli_info, long *stream_type);

int ctrlfile_set_enc_param(SHCodecs_Encoder * encoder, const char *ctrl_file);

#endif				/* CONTROL_FILE_UTIL_H */
