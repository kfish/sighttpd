/**
 * SH display
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

#define HW_ALIGN 2
#define RGB_BPP 2

struct DISPLAY {
	int fb_handle;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	unsigned long fb_base;
	unsigned long back_buf_phys;
	unsigned char *iomem;
	int fb_index;
	int lcd_w;
	int lcd_h;

	int fullscreen;
	int out_w;
	int out_h;
	int out_x;
	int out_y;

	SHVEU *veu;
};


DISPLAY *display_open(void)
{
	const char *device;
	DISPLAY *disp;
	int size;

	disp = calloc(1, sizeof(*disp));
	if (!disp)
		return NULL;

	disp->veu = shveu_open();
	if (!disp->veu) {
		free(disp);
		return NULL;
	}

	/* Initialize display */
	device = getenv("FRAMEBUFFER");
	if (!device) {
		if (access("/dev/.devfsd", F_OK) == 0) {
			device = "/dev/fb/0";
		} else {
			device = "/dev/fb0";
		}
	}

	if ((disp->fb_handle = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "Open %s: %s.\n", device, strerror(errno));
		free(disp);
		return 0;
	}
	if (ioctl(disp->fb_handle, FBIOGET_FSCREENINFO, &disp->fb_fix) < 0) {
		fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
		free(disp);
		return 0;
	}
	if (ioctl(disp->fb_handle, FBIOGET_VSCREENINFO, &disp->fb_var) < 0) {
		fprintf(stderr, "Ioctl FBIOGET_VSCREENINFO error.\n");
		free(disp);
		return 0;
	}
	if (disp->fb_fix.type != FB_TYPE_PACKED_PIXELS) {
		fprintf(stderr, "Frame buffer isn't packed pixel.\n");
		free(disp);
		return 0;
	}

	/* clear framebuffer and back buffer */
	size = (RGB_BPP * disp->fb_var.xres * disp->fb_var.yres * disp->fb_var.bits_per_pixel) / 8;
	disp->iomem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, disp->fb_handle, 0);
	if (disp->iomem != MAP_FAILED) {
		memset(disp->iomem, 0, size);
	}

	disp->lcd_w = disp->fb_var.xres;
	disp->lcd_h = disp->fb_var.yres;

	disp->back_buf_phys = disp->fb_base = disp->fb_fix.smem_start;
	disp->fb_index = 0;
	display_flip(disp);

	display_set_fullscreen(disp);

	return disp;
}

void display_close(DISPLAY *disp)
{
	disp->fb_var.xoffset = 0;
	disp->fb_var.yoffset = 0;

	/* Restore the framebuffer to the front buffer */
	ioctl(disp->fb_handle, FBIOPAN_DISPLAY, &disp->fb_var);

	close(disp->fb_handle);
	shveu_close(disp->veu);
	free(disp);
}

int display_get_format(DISPLAY *disp)
{
	return V4L2_PIX_FMT_RGB565;
}

int display_get_width(DISPLAY *disp)
{
	return disp->lcd_w;
}

int display_get_height(DISPLAY *disp)
{
	return disp->lcd_h;
}

unsigned char *display_get_back_buff_virt(DISPLAY *disp)
{
	int frame_offset = RGB_BPP * (1-disp->fb_index) * disp->lcd_w * disp->lcd_h;
	return (disp->iomem + frame_offset);
}

unsigned long display_get_back_buff_phys(DISPLAY *disp)
{
	return disp->back_buf_phys;
}

int display_flip(DISPLAY *disp)
{
	struct fb_var_screeninfo fb_screen = disp->fb_var;
	unsigned long crt = 0;

	fb_screen.xoffset = 0;
	fb_screen.yoffset = 0;
	if (disp->fb_index==0)
		fb_screen.yoffset = disp->fb_var.yres;
	if (-1 == ioctl(disp->fb_handle, FBIOPAN_DISPLAY, &fb_screen))
		return 0;

	/* Point to the back buffer */
	disp->back_buf_phys = disp->fb_base;
	if (disp->fb_index!=0)
		disp->back_buf_phys += disp->fb_fix.line_length * disp->fb_var.yres;

	disp->fb_index = (disp->fb_index+1) & 1;

	/* wait for vsync interrupt */
	ioctl(disp->fb_handle, FBIO_WAITFORVSYNC, &crt);

	return 1;
}


int display_update(
	DISPLAY *disp,
	unsigned long py,
	unsigned long pc,
	int w,
	int h,
	int pitch,
	int v4l_fmt)
{
	float scale, aspect_x, aspect_y;
	int dst_w, dst_h;
	int x1, y1, x2, y2;
	int ret;

	if (disp->fullscreen) {
		/* Stick with the source aspect ratio */
		aspect_x = (float) disp->lcd_w / w;
		aspect_y = (float) disp->lcd_h / h;
		if (aspect_x > aspect_y) {
			scale = aspect_y;
		} else {
			scale = aspect_x;
		}

		dst_w = (int) (w * scale);
		dst_h = (int) (h * scale);

		x1 = disp->lcd_w/2 - dst_w/2;
		y1 = disp->lcd_h/2 - dst_h/2;
		x2 = x1 + dst_w;
		y2 = y1 + dst_h;
	} else {
		x1 = disp->out_x;
		y1 = disp->out_y;
		x2 = x1 + disp->out_w;
		y2 = y1 + disp->out_h;
	}

	shveu_crop(disp->veu, 1, x1, y1, x2, y2);

	ret = shveu_rescale(disp->veu,
		py, pc,	(long) w, (long) h, v4l_fmt,
		disp->back_buf_phys, 0, disp->lcd_w, disp->lcd_h, V4L2_PIX_FMT_RGB565);

	if (!ret)
		display_flip(disp);

	return ret;
}

void display_set_fullscreen(DISPLAY *disp)
{
	disp->fullscreen = 1;
}

void display_set_position(DISPLAY *disp, int w, int h, int x, int y)
{
	disp->fullscreen = 0;
	disp->out_w = w;
	disp->out_h = h;
	disp->out_x = x;
	disp->out_y = y;
}

