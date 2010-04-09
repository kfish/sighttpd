/**
 * SH display
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
 *
 * Phil Edworthy <phil.edworthy@renesas.com>
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/videodev2.h>	/* For pixel formats */
#include <shveu/shveu.h>

#include "display.h"

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))

struct display_t {
	int fb_handle;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	unsigned char *fb_base;
	unsigned char *fb_screenMem;
	int fb_index;
	int lcd_w;
	int lcd_h;

	int out_w;
	int out_h;
	int out_x;
	int out_y;

	int veu;
};

/* Local functions */
static int display_flip(void *disp);



void *display_open(int veu)
{
	const char *device;
	struct display_t *pvt;
	void *iomem;
	int size;

	pvt = calloc(1, sizeof(*pvt));
	if (!pvt)
		return NULL;
	pvt->veu = veu;

	/* Initialize display */
	device = getenv("FRAMEBUFFER");
	if (!device) {
		if (access("/dev/.devfsd", F_OK) == 0) {
			device = "/dev/fb/0";
		} else {
			device = "/dev/fb0";
		}
	}

	if ((pvt->fb_handle = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "Open %s: %s.\n", device, strerror(errno));
		free(pvt);
		return 0;
	}
	if (ioctl(pvt->fb_handle, FBIOGET_FSCREENINFO, &pvt->fb_fix) < 0) {
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
		free(pvt);
		return 0;
	}
	if (ioctl(pvt->fb_handle, FBIOGET_VSCREENINFO, &pvt->fb_var) < 0) {
		fprintf(stderr, "Ioctl FBIOGET_VSCREENINFO error.\n");
		free(pvt);
		return 0;
	}
	if (pvt->fb_fix.type != FB_TYPE_PACKED_PIXELS) {
		fprintf(stderr, "Frame buffer isn't packed pixel.\n");
		free(pvt);
		return 0;
	}

	/* clear framebuffer and back buffer */
	size = (2 * pvt->fb_var.xres * pvt->fb_var.yres * pvt->fb_var.bits_per_pixel) / 8;
	iomem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, pvt->fb_handle, 0);
	if (iomem != MAP_FAILED) {
		memset(iomem, 0, size);
		munmap(iomem, size);
	}

	/* clear framebuffer (in case we couldn't mmap 2 x LCD) */
	size = (pvt->fb_var.xres * pvt->fb_var.yres * pvt->fb_var.bits_per_pixel) / 8;
	iomem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, pvt->fb_handle, 0);
	if (iomem != MAP_FAILED) {
		memset(iomem, 0, size);
		munmap(iomem, size);
	}

	pvt->lcd_w = pvt->fb_var.xres;
	pvt->lcd_h = pvt->fb_var.yres;

	pvt->fb_screenMem = pvt->fb_base = (unsigned char*)pvt->fb_fix.smem_start;
	pvt->fb_index = 0;
	display_flip(pvt);

	display_set_fullscreen(pvt);

	return pvt;
}

void display_close(void *p_display)
{
	struct display_t *pvt = (struct display_t *)p_display;

	pvt->fb_var.xoffset = 0;
	pvt->fb_var.yoffset = 0;

	/* Restore the framebuffer to the front buffer */
	ioctl(pvt->fb_handle, FBIOPAN_DISPLAY, &pvt->fb_var);

	close(pvt->fb_handle);
	free(pvt);
}

static int display_flip(void *p_display)
{
	struct display_t *pvt = (struct display_t *)p_display;
	struct fb_var_screeninfo fb_screen = pvt->fb_var;

	fb_screen.xoffset = 0;
	fb_screen.yoffset = 0;
	if (pvt->fb_index==0)
		fb_screen.yoffset = pvt->fb_var.yres;
	if (-1 == ioctl(pvt->fb_handle, FBIOPAN_DISPLAY, &fb_screen))
		return 0;

	/* Point to the back buffer */
	pvt->fb_screenMem = pvt->fb_base;
	if (pvt->fb_index!=0)
		pvt->fb_screenMem += pvt->fb_fix.line_length * pvt->fb_var.yres;

	pvt->fb_index = (pvt->fb_index+1) & 1;

	return 1;
}

/* TODO Fixed at YCbCr420 to RGB565 fullscreen */
int display_update(
	void *p_display,
	unsigned long py,
	unsigned long pc,
	int w,
	int h,
	int pitch,
	int v4l_fmt)
{
	struct display_t *pvt = (struct display_t *)p_display;
	unsigned long fb_addr;
	double scale, aspect_x, aspect_y;
	int dst_w, dst_h;
	int ret;
	unsigned long crt = 0;

	if (v4l_fmt != V4L2_PIX_FMT_NV12)
		return -1;

	if ((pvt->out_w == -1) && (pvt->out_h == -1)) {
		/* Fullscreen - stick with the source aspect ratio */
		aspect_x = (double) pvt->lcd_w / (double) w;
		aspect_y = (double) pvt->lcd_h / (double) h;
		if (aspect_x > aspect_y) {
			scale = aspect_y;
		} else {
			scale = aspect_x;
		}

		dst_w = (long) ((double) w * scale);
		dst_h = (long) ((double) h * scale);
	} else {
		dst_w = pvt->out_w;
		dst_h = pvt->out_h;
	}
	/* Observe hardware alignment */
	dst_w = dst_w - (dst_w % 4);
	dst_h = dst_h - (dst_h % 4);

	if ((pvt->out_x == -1) && (pvt->out_y == -1)) {
		/* Center - Assuming 2 bytes per pixel */
		fb_addr = (unsigned long)pvt->fb_screenMem
			+ (pvt->lcd_w - dst_w)
			+ (pvt->lcd_h - dst_h) * pvt->lcd_w;
	} else {
		fb_addr = (unsigned long)pvt->fb_screenMem
			+ (pvt->out_x + (pvt->out_y * pvt->lcd_w)) * 2;
	}

	shveu_operation(pvt->veu,
			py, pc,	(long) w, (long) h, (long) w, SHVEU_YCbCr420,
			fb_addr, 0UL, dst_w, dst_h, pvt->lcd_w, SHVEU_RGB565,
			SHVEU_NO_ROT);

	display_flip(pvt);

	/* wait for vsync interrupt */
	ret = ioctl(pvt->fb_handle, FBIO_WAITFORVSYNC, &crt);

	return ret;
}

int display_set_fullscreen(void *p_display)
{
	struct display_t *pvt = (struct display_t *)p_display;
	pvt->out_w = -1;
	pvt->out_h = -1;
	pvt->out_x = -1;
	pvt->out_y = -1;
}

int display_set_position(void *p_display, int w, int h, int x, int y)
{
	struct display_t *pvt = (struct display_t *)p_display;
	pvt->out_w = w;
	pvt->out_h = h;
	pvt->out_x = x;
	pvt->out_y = y;

	/* Ensure the output is no bigger than the LCD */
	pvt->out_x = min(pvt->out_x, pvt->lcd_w);
	pvt->out_y = min(pvt->out_y, pvt->lcd_h);
	pvt->out_w = min(pvt->out_w, pvt->lcd_w - pvt->out_x);
	pvt->out_h = min(pvt->out_h, pvt->lcd_h - pvt->out_y);
}

