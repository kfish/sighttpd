/**
 * SH display
 * Helper to send YCbCr420 frames to the frame buffer. It uses the VEU
 * hardware to scale & color convert the frames. it also handles aspect
 * ratios for you and double buffered screens.
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

#include <linux/videodev2.h>	/* For pixel formats */

/**
 * Open the frame buffer
 * \param veu Handle to VEU driver (opened with shveu_open)
 * \retval 0 Failure
 * \retval >0 Handle
 */
void *display_open(int veu);

/**
 * Close the frame buffer
 * \param p_display Handle returned from display_open
 */
void display_close(void *p_display);

/**
 * Position the output fullscreen (but observe aspect ratio)
 * \param p_display Handle returned from display_open
 */
int display_set_fullscreen(void *p_display);

/**
 * Explicity position the output
 * \param p_display Handle returned from display_open
 */
int display_set_position(void *p_display, int w, int h, int x, int y);

/**
 * Send frame to the framebuffer
 * \param p_display Handle returned from display_open
 * \param py Physical address of Y or RGB plane of source image
 * \param pc Physical address of CbCr plane of source image (ignored for RGB)
 * \param w Width in pixels of source image
 * \param h Height in pixels of source image
 * \param pitch Line pitch of source image
 * \param v4l_fmt Format of source image (see <linux/videodev2.h>)
 */
int display_update(
	void *p_display,
	unsigned long py,
	unsigned long pc,
	int w,
	int h,
	int pitch,
	int v4l_fmt
);

