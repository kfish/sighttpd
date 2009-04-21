
bin_PROGRAMS = camserv

noinst_HEADERS = \
        list.h \
        params.h \
        http.h \
        tests.h

camserv_SOURCES = \
	main.c \
	list.c \
	params.c \
        http.c

camserv_LDFLAGS = $(PTHREAD_LIBS) $(RT_LIBS)

# Unit tests
test: check

TESTS = test_params

noinst_PROGRAMS = $(TESTS)

test_params_SOURCES = list.c params.c test_params.c
