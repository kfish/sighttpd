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

/*
 * This program captures v4l2 input (e.g. from a camera), optionally crops
 * and rotates this, encodes this and shows it on the display.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <stdarg.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/videodev2.h>	/* For pixel formats */
#include <linux/ioctl.h>
#include <pthread.h>
#include <errno.h>

#include <uiomux/uiomux.h>
#include <shveu/shveu.h>
#include <shcodecs/shcodecs_encoder.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"

#include "avcbencsmp.h"
#include "capture.h"
#include "ControlFileUtil.h"
#include "framerate.h"
#include "display.h"
#include "thrqueue.h"

#define DEBUG

/* Maximum number of cameras handled by the program */
#define MAX_CAMERAS 8

/* Maximum number of encoders per camera */
#define MAX_ENCODERS 8

struct camera_data {
	char * devicename;

	/* Captured frame information */
	capture *ceu;
	unsigned long cap_w;
	unsigned long cap_h;
	int captured_frames;

	pthread_t convert_thread;
	pthread_t capture_thread;
	struct Queue * captured_queue;
	pthread_mutex_t capture_start_mutex;

	struct framerate * cap_framerate;
};

struct encode_data {
	char * path;
	char * ctlfile;

	char ctrl_filename[MAXPATHLEN];
	APPLI_INFO ainfo;

	struct camera_data * camera;

	long stream_type;

	pthread_mutex_t encode_start_mutex;
	FILE *output_fp;

	unsigned long enc_w;
	unsigned long enc_h;

	struct framerate * enc_framerate;
};

struct private_data {
	UIOMux * uiomux;

	int nr_cameras;
	struct camera_data cameras[MAX_CAMERAS];

	int nr_encoders;
	SHCodecs_Encoder *encoders[MAX_ENCODERS];
	struct encode_data encdata[MAX_ENCODERS];

	int do_preview;
	void *display;

	int rotate_cap;

	int output_frames;
};

void debug_printf(const char *fmt, ...)
{
#ifdef DEBUG
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
#endif
}

struct private_data pvt_data;

static int alive=1;

/* XXX: from avcbeinputuser.c */
FILE *
open_output_file(const char *fname)
{
	FILE *fp = NULL;

	if (!strcmp (fname, "-"))
		fp = stdout;
	else
		fp = fopen(fname, "wb");

	return fp;
}

/* close output file */
void close_output_file(FILE *fp)
{
	if (fp != NULL) {
		fflush(fp);
		fclose(fp);
	}
}

/*****************************************************************************/

/* Callback for frame capture */
static void
capture_image_cb(capture *ceu, const unsigned char *frame_data, size_t length,
		 void *user_data)
{
	struct camera_data *cam = (struct camera_data*)user_data;

	queue_enq (cam->captured_queue, frame_data);
	cam->captured_frames++;
}

void *capture_main(void *data)
{
	struct camera_data *cam = (struct camera_data*)data;

	while(alive){
		framerate_wait(cam->cap_framerate);
		capture_get_frame(cam->ceu, capture_image_cb, cam);

		/* This mutex is unlocked once the capture buffer is free */
		pthread_mutex_lock(&cam->capture_start_mutex);
	}

	capture_stop_capturing(cam->ceu);

	pthread_mutex_unlock(&cam->capture_start_mutex);

	return NULL;
}


void *convert_main(void *data)
{
	struct camera_data *cam = (struct camera_data*)data;
	struct private_data *pvt = &pvt_data;
	int pitch, offset;
	void *ptr;
	unsigned long enc_y, enc_c;
	unsigned long cap_y, cap_c;
	int i;

	while(alive){
		cap_y = (unsigned long) queue_deq(cam->captured_queue);
		cap_c = cap_y + (cam->cap_w * cam->cap_h);

		for (i=0; i < pvt->nr_encoders; i++) {
			if (pvt->encdata[i].camera != cam) continue;

			shcodecs_encoder_get_input_physical_addr (pvt->encoders[i], (unsigned int *)&enc_y, (unsigned int *)&enc_c);

			/* We are clipping, not scaling, as we need to perform a rotation,
		   	but the VEU cannot do a rotate & scale at the same time. */
			uiomux_lock (pvt->uiomux, UIOMUX_SH_VEU);
			shveu_operation(0,
				cap_y, cap_c,
				cam->cap_w, cam->cap_h, cam->cap_w, SHVEU_YCbCr420,
				enc_y, enc_c,
				pvt->encdata[i].enc_w, pvt->encdata[i].enc_h, pvt->encdata[i].enc_w, SHVEU_YCbCr420,
				pvt->rotate_cap);
			uiomux_unlock (pvt->uiomux, UIOMUX_SH_VEU);

			/* Let the encoder get_input function return */
			pthread_mutex_unlock(&pvt->encdata[i].encode_start_mutex);
		}

		if (cam == pvt->encdata[0].camera && pvt->do_preview) {
			/* Use the VEU to scale the capture buffer to the frame buffer */
			uiomux_lock (pvt->uiomux, UIOMUX_SH_VEU);
			display_update(pvt->display,
					cap_y, cap_c,
					cam->cap_w, cam->cap_h, cam->cap_w,
					V4L2_PIX_FMT_NV12);
			uiomux_unlock (pvt->uiomux, UIOMUX_SH_VEU);
		}

		capture_queue_buffer (cam->ceu, cap_y);
		pthread_mutex_unlock(&cam->capture_start_mutex);

		pvt->output_frames++;
	}

	return NULL;
}

/* SHCodecs_Encoder_Input callback for acquiring an image */
static int get_input(SHCodecs_Encoder *encoder, void *user_data)
{
	struct encode_data *encdata = (struct encode_data*)user_data;

	/* This mutex is unlocked once the capture buffer has been copied to the
	   encoder input buffer */
	pthread_mutex_lock(&encdata->encode_start_mutex);

	if (encdata->enc_framerate == NULL) {
		encdata->enc_framerate = framerate_new_measurer ();
	}

	return (alive?0:1);
}

/* SHCodecs_Encoder_Output callback for writing out the encoded data */
static int write_output(SHCodecs_Encoder *encoder,
			unsigned char *data, int length, void *user_data)
{
	struct encode_data *encdata = (struct encode_data*)user_data;
	double ifps, mfps;

	if (shcodecs_encoder_get_frame_num_delta(encoder) > 0 &&
			encdata->enc_framerate != NULL) {
		if (encdata->enc_framerate->nr_handled >= encdata->ainfo.frames_to_encode &&
				encdata->ainfo.frames_to_encode > 0)
			return 1;
		framerate_mark (encdata->enc_framerate);
		ifps = framerate_instantaneous_fps (encdata->enc_framerate);
		mfps = framerate_mean_fps (encdata->enc_framerate);
		if (encdata->enc_framerate->nr_handled % 10 == 0) {
			fprintf (stderr, "  Encoding @ %4.2f fps \t(avg %4.2f fps)\r", ifps, mfps);
		}
	}

	if (fwrite(data, 1, length, encdata->output_fp) < (size_t)length)
		return -1;

	return (alive?0:1);
}

void cleanup (void)
{
	double time;
	struct private_data *pvt = &pvt_data;
	int i;

	for (i=0; i < pvt->nr_cameras; i++) {
		struct camera_data * cam = &pvt->cameras[i];

		time = (double)framerate_elapsed_time (cam->cap_framerate);
		time /= 1000000;

		debug_printf ("\n");
		debug_printf("Elapsed time (capture): %0.3g s\n", time);

		debug_printf("Captured %d frames (%.2f fps)\n", cam->captured_frames,
			 	(double)cam->captured_frames/time);
		if (pvt->do_preview) {
			debug_printf("Displayed %d frames (%.2f fps)\n", pvt->output_frames,
					(double)pvt->output_frames/time);
		}

		framerate_destroy (cam->cap_framerate);
	}

	for (i=0; i < pvt->nr_encoders; i++) {
		time = (double)framerate_elapsed_time (pvt->encdata[i].enc_framerate);
		time /= 1000000;

		debug_printf("[%d] Elapsed time (encode): %0.3g s\n", i, time);
		debug_printf("[%d] Encoded %d frames (%.2f fps)\n", i,
				pvt->encdata[i].enc_framerate->nr_handled,
			 	framerate_mean_fps (pvt->encdata[i].enc_framerate));

		shcodecs_encoder_close(pvt->encoders[i]);

		framerate_destroy (pvt->encdata[i].enc_framerate);
	}

	alive=0;

	for (i=0; i < pvt->nr_cameras; i++) {
		struct camera_data * cam = &pvt->cameras[i];

		pthread_join (cam->convert_thread, NULL);
		pthread_join (cam->capture_thread, NULL);

		capture_close(cam->ceu);
	}

	if (pvt->do_preview)
		display_close(pvt->display);
	shveu_close();

	for (i=0; i < pvt->nr_encoders; i++) {
		close_output_file(pvt->encdata[i].output_fp);
		pthread_mutex_unlock(&pvt->encdata[i].encode_start_mutex);
		pthread_mutex_destroy (&pvt->encdata[i].encode_start_mutex);
	}

	for (i=0; i < pvt->nr_cameras; i++) {
		struct camera_data * cam = &pvt->cameras[i];
		pthread_mutex_destroy (&cam->capture_start_mutex);
	}

	uiomux_close (pvt->uiomux);
}

struct camera_data * get_camera (char * devicename, int width, int height)
{
	struct private_data *pvt = &pvt_data;
	int i;

	for (i=0; i < MAX_CAMERAS; i++) {
		if (pvt->cameras[i].devicename == NULL)
			break;

		if (!strcmp (pvt->cameras[i].devicename, devicename)) {
			return &pvt->cameras[i];
		}
	}

	if (i == MAX_CAMERAS) return NULL;

	pvt->cameras[i].devicename = devicename;
	pvt->cameras[i].cap_w = width;
	pvt->cameras[i].cap_h = height;

	pvt->nr_cameras = i+1;

	return &pvt->cameras[i];
}

int shrecord_run (void)
{
	struct private_data *pvt;
	int return_code, rc;
	unsigned int pixel_format;
	int c, i=0;
	long target_fps10;
	unsigned long rotate_input;

	pvt = &pvt_data;

	memset (pvt, 0, sizeof(struct private_data));

	pvt->do_preview = 1;

	pvt->output_frames = 0;
	pvt->rotate_cap = SHVEU_NO_ROT;

	/* XXX: pvt->nr_encoders = i; */

	pvt->uiomux = uiomux_open ();

	for (i=0; i < pvt->nr_encoders; i++) {
		if ( (strcmp(pvt->encdata[i].ctrl_filename, "-") == 0) ||
				(pvt->encdata[i].ctrl_filename[0] == '\0') ){
			fprintf(stderr, "Invalid v4l2 configuration file.\n");
			return -1;
		}

		return_code = ctrlfile_get_params(pvt->encdata[i].ctrl_filename,
				&pvt->encdata[i].ainfo, &pvt->encdata[i].stream_type);
		if (return_code < 0) {
			fprintf(stderr, "Error opening control file %s.\n", pvt->encdata[i].ctrl_filename);
			return -2;
		}

		pvt->encdata[i].camera = get_camera (pvt->encdata[i].ainfo.input_file_name_buf, pvt->encdata[i].ainfo.xpic, pvt->encdata[i].ainfo.ypic);

		debug_printf("[%d] Input file: %s\n", i, pvt->encdata[i].ainfo.input_file_name_buf);
		debug_printf("[%d] Output file: %s\n", i, pvt->encdata[i].ainfo.output_file_name_buf);

		pthread_mutex_init(&pvt->encdata[i].encode_start_mutex, NULL);
		pthread_mutex_unlock(&pvt->encdata[i].encode_start_mutex);
	}

	for (i=0; i < pvt->nr_cameras; i++) {
		/* Initalise the mutexes */
		pthread_mutex_init(&pvt->cameras[i].capture_start_mutex, NULL);
		pthread_mutex_lock(&pvt->cameras[i].capture_start_mutex);

		/* Initialize the queues */
		pvt->cameras[i].captured_queue = queue_init();
		queue_limit (pvt->cameras[i].captured_queue, 2);

		/* Create the threads */
		rc = pthread_create(&pvt->cameras[i].convert_thread, NULL, convert_main, &pvt->cameras[i]);
		if (rc) {
			fprintf(stderr, "pthread_create failed, exiting\n");
			return -7;
		}

		/* Camera capture initialisation */
		pvt->cameras[i].ceu = capture_open_userio(pvt->cameras[i].devicename, pvt->cameras[i].cap_w, pvt->cameras[i].cap_h);
		if (pvt->cameras[i].ceu == NULL) {
			fprintf(stderr, "capture_open failed, exiting\n");
			return -3;
		}
		capture_set_use_physical(pvt->cameras[i].ceu, 1);
		pvt->cameras[i].cap_w = capture_get_width(pvt->cameras[i].ceu);
		pvt->cameras[i].cap_h = capture_get_height(pvt->cameras[i].ceu);

		pixel_format = capture_get_pixel_format (pvt->cameras[i].ceu);
		if (pixel_format != V4L2_PIX_FMT_NV12) {
			fprintf(stderr, "Camera capture pixel format is not supported\n");
			return -4;
		}
		debug_printf("Camera %d resolution:  %dx%d\n", i, pvt->cameras[i].cap_w, pvt->cameras[i].cap_h);
	}

	/* VEU initialisation */
	if (shveu_open() < 0) {
		fprintf (stderr, "Could not open VEU, exiting\n");
	}

	if (pvt->do_preview) {
		pvt->display = display_open(0);
		if (!pvt->display) {
			return -5;
		}
	}

	for (i=0; i < pvt->nr_encoders; i++) {
#if 0
		if (pvt->rotate_cap == SHVEU_NO_ROT) {
			pvt->encdata[i].enc_w = pvt->cap_w;
			pvt->encdata[i].enc_h = pvt->cap_h;
		} else {
			pvt->encdata[i].enc_w = pvt->cap_h;
			pvt->encdata[i].enc_h = pvt->cap_h * pvt->cap_h / pvt->cap_w;
			/* Round down to nearest multiple of 16 for VPU */
			pvt->encdata[i].enc_w = pvt->encdata[i].enc_w - (pvt->encdata[i].enc_w % 16);
			pvt->encdata[i].enc_h = pvt->encdata[i].enc_h - (pvt->encdata[i].enc_h % 16);
			debug_printf("[%d] Rotating & cropping camera image ...\n", i);
		}
#else
		/* Override the encoding frame size in case of rotation */
		if (pvt->rotate_cap == SHVEU_NO_ROT) {
			pvt->encdata[i].enc_w = pvt->encdata[i].ainfo.xpic;
			pvt->encdata[i].enc_h = pvt->encdata[i].ainfo.ypic;
		} else {
			pvt->encdata[i].enc_w = pvt->encdata[i].ainfo.ypic;
			pvt->encdata[i].enc_h = pvt->encdata[i].ainfo.xpic;
		}
		debug_printf("[%d] Encode resolution:  %dx%d\n", i, pvt->encdata[i].enc_w, pvt->encdata[i].enc_h);
#endif

		/* VPU Encoder initialisation */
		pvt->encdata[i].output_fp = open_output_file(pvt->encdata[i].ainfo.output_file_name_buf);
		if (pvt->encdata[i].output_fp == NULL) {
			fprintf(stderr, "Error opening output file\n");
			return -8;
		}

		pvt->encoders[i] = shcodecs_encoder_init(pvt->encdata[i].enc_w, pvt->encdata[i].enc_h, pvt->encdata[i].stream_type);
		if (pvt->encoders[i] == NULL) {
			fprintf(stderr, "shcodecs_encoder_init failed, exiting\n");
			return -5;
		}
		shcodecs_encoder_set_input_callback(pvt->encoders[i], get_input, &pvt->encdata[i]);
		shcodecs_encoder_set_output_callback(pvt->encoders[i], write_output, &pvt->encdata[i]);

		return_code = ctrlfile_set_enc_param(pvt->encoders[i], pvt->encdata[i].ctrl_filename);
		if (return_code < 0) {
			fprintf (stderr, "Problem with encoder params in control file!\n");
			return -9;
		}

		//shcodecs_encoder_set_xpic_size(pvt->encoders[i], pvt->encdata[i].enc_w);
		//shcodecs_encoder_set_ypic_size(pvt->encoders[i], pvt->encdata[i].enc_h);
	}

	/* Set up the frame rate timer to match the encode framerate */
	target_fps10 = shcodecs_encoder_get_frame_rate(pvt->encoders[0]);
	fprintf (stderr, "Target framerate:   %.1f fps\n", target_fps10 / 10.0);

	for (i=0; i < pvt->nr_cameras; i++) {
		/* Initialize framerate timer */
		pvt->cameras[i].cap_framerate = framerate_new_timer (target_fps10 / 10.0);

		capture_start_capturing(pvt->cameras[i].ceu);

		rc = pthread_create(&pvt->cameras[i].capture_thread, NULL, capture_main, &pvt->cameras[i]);
		if (rc){
			fprintf(stderr, "pthread_create failed, exiting\n");
			return -6;
		}
	}

	rc = shcodecs_encoder_run_multiple(pvt->encoders, pvt->nr_encoders);
	if (rc < 0) {
		fprintf(stderr, "Error encoding, error code=%d\n", rc);
		rc = -10;
	}
	/* Exit ok if shcodecs_encoder_run was stopped cleanly */
	if (rc == 1) rc = 0;

	cleanup ();

	return rc;

exit_err:
	/* General exit, prior to thread creation */
	exit (1);
}

static int
shrecord_check (http_request * request, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;

        return !strncmp (request->path, ed->path, strlen(ed->path));
}

#define SHRECORD_STATICTEXT "<<< SHRecord >>>"

static void
shrecord_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;
	params_t * r = *response_headers;
        char length[16];

        *status_line = http_status_line (HTTP_STATUS_OK);

        r = params_append (r, "Content-Type", "video/mp4");
        snprintf (length, 16, "%d", strlen (SHRECORD_STATICTEXT));
        *response_headers = params_append (r, "Content-Length", length);
}

static void
shrecord_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct encode_data * ed = (struct encode_data *)data;

        write (fd, SHRECORD_STATICTEXT, strlen(SHRECORD_STATICTEXT));
}

static void
shrecord_delete (void * data)
{
	struct encode_data * ed = (struct encode_data *)data;

	free (ed);
}

struct resource *
shrecord_resource (char * path, char * ctlfile)
{
	struct encode_data * ed;

	if ((ed = calloc (1, sizeof(*ed))) == NULL)
		return NULL;

	ed->path = path;
	ed->ctlfile = ctlfile;

	return resource_new (shrecord_check, shrecord_head, shrecord_body, shrecord_delete, ed);
}

list_t *
shrecord_resources (Dictionary * config)
{
	list_t * l;
	const char * path;
	const char * ctlfile;

	l = list_new();

	path = dictionary_lookup (config, "Path");
	ctlfile = dictionary_lookup (config, "CtlFile");

	if (path && ctlfile)
		l = list_append (l, shrecord_resource (path, ctlfile));

	return l;
}
