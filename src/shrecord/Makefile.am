## Process this file with automake to produce Makefile.in

if HAVE_SHCODECS
if HAVE_SHVEU
if HAVE_UIOMUX

INCLUDES = -I$(top_builddir) \
           -I$(top_srcdir)/include

noinst_LIBRARIES = libshrecord.a

noinst_HEADERS = \
	avcbencsmp.h \
	capture.h \
	framerate.h \
	thrqueue.h \
	ControlFileUtil.h

libshrecord_a_SOURCES = \
	shrecord.c \
	capture.c \
	display.c \
	framerate.c \
	thrqueue.c \
	ControlFileUtil.c

libshrecord_a_CFLAGS = $(SHCODECS_CFLAGS) $(SHVEU_CFLAGS) $(UIOMUX_CFLAGS)

endif
endif
endif
