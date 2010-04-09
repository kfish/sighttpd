/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>	      /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <uiomux/uiomux.h>
#include "capture.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define PAGE_ALIGN(addr) (((addr) + getpagesize() - 1) & ~(getpagesize()-1))

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void *start;              	/* User space addr */
	unsigned char *phys_addr;	/* Only used for USERPTR I/O */
	size_t length;
	struct v4l2_buffer v4l2buf;
};

struct capture_ {
	const char *dev_name;
	int fd;
	io_method io;
	struct buffer *buffers;
	unsigned int n_buffers;
	int width;
	int height;
	unsigned int pixel_format;
	int use_physical;
	UIOMux *uiomux;
} sh_ceu;


static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));

	exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
	int r;

	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

static int
read_frame(capture * cap, capture_callback cb, void *user_data)
{
	struct v4l2_buffer buf;
	unsigned int i;

	CLEAR(buf);

	switch (cap->io) {
	case IO_METHOD_READ:
		if (-1 == read(cap->fd, cap->buffers[0].start, cap->buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("read");
			}
		}

		cb(cap, cap->buffers[0].start, cap->buffers[0].length, user_data);

		break;

	case IO_METHOD_MMAP:
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(cap->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < cap->n_buffers);
		memcpy(&cap->buffers[buf.index].v4l2buf, &buf, sizeof(struct v4l2_buffer));
		cb(cap, cap->buffers[buf.index].start,
		   cap->buffers[buf.index].length, user_data);

		break;

	case IO_METHOD_USERPTR:
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(cap->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < cap->n_buffers; ++i){
			/* TODO Work around the kernel - it sets the buffer size incorrectly */
			buf.length = cap->buffers[i].length;

			if (buf.m.userptr == (unsigned long) cap->buffers[i].start
			    && buf.length == cap->buffers[i].length)
				break;
		}
		assert(i < cap->n_buffers);
		memcpy(&cap->buffers[buf.index].v4l2buf, &buf, sizeof(struct v4l2_buffer));

		if (cap->use_physical)
			cb(cap, cap->buffers[i].phys_addr, buf.length, user_data);
		else
			cb(cap, cap->buffers[i].start, buf.length, user_data);

		break;
	}

	return 1;
}

void
capture_queue_buffer(capture * cap, const void * buffer_data)
{
	int i;

	if (cap->use_physical) {
		for (i = 0; i < cap->n_buffers; ++i) {
			if (cap->buffers[i].phys_addr == buffer_data) {
				goto found;
			}
		}
	} else {
		for (i = 0; i < cap->n_buffers; ++i) {
			if (cap->buffers[i].start == buffer_data) {
				goto found;
			}
		}
	}

	fprintf (stderr, "%s: Tried to release bad buffer %d %p\n", __func__, i, buffer_data);
	exit (EXIT_FAILURE);

found:
	if (-1 == xioctl(cap->fd, VIDIOC_QBUF, &cap->buffers[i].v4l2buf))
		errno_exit("VIDIOC_QBUF");
}

int
capture_set_use_physical(capture * cap, int use_physical)
{
	if (!cap) return -1;

	cap->use_physical = use_physical;
	return 0;
}

void
capture_get_frame(capture * cap, capture_callback cb, void *user_data)
{

	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(cap->fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(cap->fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			errno_exit("select");
		}

		if (0 == r) {
			fprintf(stderr, "select timeout\n");
			exit(EXIT_FAILURE);
		}

		if (read_frame(cap, cb, user_data))
			break;
		/* EAGAIN - continue select loop. */
	}
}

void capture_stop_capturing(capture * cap)
{
	enum v4l2_buf_type type;

	switch (cap->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(cap->fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
}

void capture_start_capturing(capture * cap)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (cap->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cap->n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = i;

			if (-1 == xioctl(cap->fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(cap->fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < cap->n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr   = (unsigned long) cap->buffers[i].start;
			buf.length      = cap->buffers[i].length;

			if (-1 == xioctl(cap->fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(cap->fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	}
}

static void uninit_device(capture * cap)
{
	unsigned int i;

	switch (cap->io) {
	case IO_METHOD_READ:
		free(cap->buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cap->n_buffers; ++i)
			if (-1 == 
				munmap(cap->buffers[i].start, cap->buffers[i].length))
				errno_exit("munmap");
		break;

	case IO_METHOD_USERPTR:
		/* UIO memory, not stuff we malloc'ed */
		break;
	}

	free(cap->buffers);
}

static void init_read(capture * cap, unsigned int buffer_size)
{
	cap->buffers = calloc (1, sizeof (*cap->buffers));

	if (!cap->buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	cap->buffers[0].length = buffer_size;
	cap->buffers[0].start = malloc (buffer_size);

	if (!cap->buffers[0].start) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
}

static void init_mmap(capture * cap)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 2;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(cap->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "memory mapping\n", cap->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			 cap->dev_name);
		exit(EXIT_FAILURE);
	}

	cap->buffers = calloc(req.count, sizeof(*cap->buffers));

	if (!cap->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (cap->n_buffers = 0; cap->n_buffers < req.count;
	     ++cap->n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = cap->n_buffers;

		if (-1 == xioctl(cap->fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		cap->buffers[cap->n_buffers].length = buf.length;
		cap->buffers[cap->n_buffers].start =
			mmap(NULL /* start anywhere */ ,
			      buf.length,
			      PROT_READ | PROT_WRITE /* required */ ,
			      MAP_SHARED /* recommended */ ,
			      cap->fd, buf.m.offset);

		if (MAP_FAILED == cap->buffers[cap->n_buffers].start)
			errno_exit("mmap");
	}
}

static void
init_userp(capture * cap, unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();

	CLEAR(req);

	req.count	= 2;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(cap->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "user pointer i/o\n", cap->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			 cap->dev_name);
		exit(EXIT_FAILURE);
	}

	cap->buffers = calloc(req.count, sizeof (*cap->buffers));

	if (!cap->buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (cap->n_buffers = 0; cap->n_buffers < req.count; ++cap->n_buffers) {
		void *user;
		unsigned long phys;

		/* Get user memory from UIO */
		user = uiomux_malloc(cap->uiomux, UIOMUX_SH_VEU,
               buffer_size, page_size);
		phys = uiomux_virt_to_phys(cap->uiomux, UIOMUX_SH_VEU, user);

		cap->buffers[cap->n_buffers].length = buffer_size;
		cap->buffers[cap->n_buffers].start = user;
		cap->buffers[cap->n_buffers].phys_addr = (void *)phys;

		if (!cap->buffers[cap->n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(capture * cap)
{
	struct v4l2_capability capb;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(cap->fd, VIDIOC_QUERYCAP, &capb)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",
				cap->dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(capb.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",
			 cap->dev_name);
		exit(EXIT_FAILURE);
	}

	switch (cap->io) {
	case IO_METHOD_READ:
		if (!(capb.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				 cap->dev_name);
			exit(EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(capb.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n",
				 cap->dev_name);
			exit(EXIT_FAILURE);
		}

		break;
	}


	/* Select video input, video standard and tune here. */


	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(cap->fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(cap->fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {	
		/* Errors ignored. */
	}


	CLEAR(fmt);

	fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = cap->width; 
	fmt.fmt.pix.height      = cap->height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	if (-1 == xioctl(cap->fd, VIDIOC_S_FMT, &fmt)) {
		errno_exit("VIDIOC_S_FMT");
	}
	cap->pixel_format = fmt.fmt.pix.pixelformat;

	/* Note VIDIOC_S_FMT may change width and height. */
	cap->width = fmt.fmt.pix.width;
	cap->height = fmt.fmt.pix.height;

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	/* TODO Work around the kernel - it sets the buffer size incorrectly */
	fmt.fmt.pix.sizeimage = PAGE_ALIGN(fmt.fmt.pix.sizeimage);

	switch (cap->io) {
	case IO_METHOD_READ:
		init_read(cap, fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap(cap);
		break;

	case IO_METHOD_USERPTR:
		init_userp(cap, fmt.fmt.pix.sizeimage);
		break;
	}
}

static void close_device(capture * cap)
{
	uiomux_close(cap->uiomux);

	if (-1 == close(cap->fd))
		errno_exit("close");

	cap->fd = -1;
}

static void open_device(capture * cap)
{
	struct stat st; 


	if (-1 == stat(cap->dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
			cap->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", cap->dev_name);
		exit(EXIT_FAILURE);
	}

	cap->fd =
	    open(cap->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == cap->fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
			cap->dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* User mapped memory */
	cap->uiomux = uiomux_open();
	if (cap->uiomux == 0) {
		fprintf(stderr, "Cannot uiomux: %d, %s\n",
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void capture_close(capture * cap)
{
	uninit_device(cap);

	close_device(cap);

	free(cap);
}

static capture *capture_open_mode(const char *device_name, int width, int height, int mode)
{
	capture *cap;

	cap = malloc(sizeof(*cap));
	if (!cap)
		return NULL;

	cap->dev_name = device_name;
	cap->io = mode;
	cap->width = width;
	cap->height = height;
	cap->use_physical = 0;

	open_device(cap);
	init_device(cap);

	return cap;
}

capture *capture_open(const char *device_name, int width, int height)
{
	return capture_open_mode(device_name, width, height, IO_METHOD_MMAP);
}

capture *capture_open_userio(const char *device_name, int width, int height)
{
	return capture_open_mode(device_name, width, height, IO_METHOD_USERPTR);
}

int capture_get_width(capture * cap)
{
	return cap->width;
}

int capture_get_height(capture * cap)
{
	return cap->height;
}

unsigned int capture_get_pixel_format(capture * cap)
{
	return cap->pixel_format;
}

