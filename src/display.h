/**
 * SH display
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H


#include <linux/videodev2.h>	/* For pixel formats */

/**
 * An opaque handle to the display.
 */
struct DISPLAY;
typedef struct DISPLAY DISPLAY;

/**
 * Open the display
 * \retval 0 Failure
 * \retval >0 Handle
 */
DISPLAY *display_open(void);

/**
 * Close the frame buffer
 * \param disp Handle returned from display_open
 */
void display_close(DISPLAY *disp);

/**
 * Get the v4l2 format of the display
 * \param disp Handle returned from display_open
 */
int display_get_format(DISPLAY *disp);

/**
 * Get the width of the display in pixels
 * \param disp Handle returned from display_open
 */
int display_get_width(DISPLAY *disp);

/**
 * Get the height of the display in pixels
 * \param disp Handle returned from display_open
 */
int display_get_height(DISPLAY *disp);

/**
 * Get a pointer to the back buffer
 * \param disp Handle returned from display_open
 */
unsigned char *display_get_back_buff_virt(DISPLAY *disp);

/**
 * Get the physical address of the back buffer
 * \param disp Handle returned from display_open
 */
unsigned long display_get_back_buff_phys(DISPLAY *disp);

/**
 * Place the back buffer on the screen
 * \param disp Handle returned from display_open
 */
int display_flip(DISPLAY *disp);



/* The functions below are used to place an image on the display */

/**
 * Position the output fullscreen (but observe aspect ratio)
 * \param disp Handle returned from display_open
 */
void display_set_fullscreen(DISPLAY *disp);

/**
 * Explicity position the output
 * \param disp Handle returned from display_open
 */
void display_set_position(DISPLAY *disp, int w, int h, int x, int y);

/**
 * Send frame to the framebuffer
 * \param disp Handle returned from display_open
 * \param py Physical address of Y or RGB plane of source image
 * \param pc Physical address of CbCr plane of source image (ignored for RGB)
 * \param w Width in pixels of source image
 * \param h Height in pixels of source image
 * \param pitch Line pitch of source image
 * \param v4l_fmt Format of source image (see <linux/videodev2.h>)
 */
int display_update(
	DISPLAY *disp,
	unsigned long py,
	unsigned long pc,
	int w,
	int h,
	int pitch,
	int v4l_fmt
);

#endif

