
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

TESTS = httpdate_test test_http_request test_params

noinst_PROGRAMS = $(TESTS)

httpdate_test_SOURCES = httpdate.c httpdate_test.c

test_http_request_SOURCES = http.c test_http_request.c

test_params_SOURCES = list.c params.c test_params.c
