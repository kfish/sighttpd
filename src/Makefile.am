
bin_PROGRAMS = camserv

noinst_HEADERS = \
        list.h \
        params.h \
        http-date.h \
        http-reqline.h \
        tests.h

camserv_SOURCES = \
	main.c \
	list.c \
	params.c \
        http-date.c \
        http-reqline.c

camserv_LDFLAGS = $(PTHREAD_LIBS) $(RT_LIBS)

# Unit tests
test: check

TESTS = http-date_test http-reqline_test params_test

noinst_PROGRAMS = $(TESTS)

http_date_test_SOURCES = http-date.c http-date_test.c

http_reqline_test_SOURCES = http-reqline.c http-reqline_test.c

params_test_SOURCES = list.c params.c params_test.c
