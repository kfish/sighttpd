## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_builddir) \
           -I$(top_srcdir)/include

bin_PROGRAMS = shrecord

noinst_HEADERS = \
	avcbencsmp.h \
	capture.h \
	framerate.h \
	thrqueue.h \
	ControlFileUtil.h

shrecord_SOURCES = \
	shrecord.c \
	capture.c \
	display.c \
	framerate.c \
	thrqueue.c \
	ControlFileUtil.c

shrecord_CFLAGS = $(SHCODECS_CFLAGS) $(SHVEU_CFLAGS) $(UIOMUX_CFLAGS)
shrecord_LDADD = $(SHCODECS_LIBS) $(SHVEU_LIBS) $(UIOMUX_LIBS) -lrt -lpthread
