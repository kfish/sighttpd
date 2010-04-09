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

#ifndef	AVCBENCSMP_H
#define	AVCBENCSMP_H

#include <shcodecs/shcodecs_encoder.h>

/*----- structures -----*/

typedef struct {

	/* Input YUV file name and it's file pointer */
	char input_file_name_buf[256];	/* for stream-1 */
	FILE *input_yuv_fp;	/* for stream-1 */
	long yuv_CbCr_format;	/* YUVデータ内（出力するローカルデコードファイルも）
				   のCb,Crデータの並び順（1:Cb全部Cr全部、2:Cb0,Cr0,Cb1,Cr1,...、
				   3:Cbの1ライン分,Crの1ライン分,...） */

	FILE *output_file_fp;	/* for output stream-2 */
	char output_file_name_buf[256];	/* 出力m4vファイル名 */

	long xpic;
	long ypic;

	long frames_to_encode;		/* target number of frames to encode */
	long frame_counter_of_input;	/* the number of input frames encoded so far */

} APPLI_INFO;


//#define ALIGN(a, w) (void *)(((unsigned long)(a) + (w) - 1) & ~((w) - 1))


int open_input_image_file(APPLI_INFO * appli_info);
int load_1frame_from_image_file(SHCodecs_Encoder * encoder, APPLI_INFO * appli_info);
void close_input_image_file(APPLI_INFO * appli_info);


FILE *
open_output_file(const char *fname);

void close_output_file(FILE *fp);


#endif				/* AVCBENCSMP */
