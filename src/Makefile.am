
bin_PROGRAMS = sighttpd

# Data structures
ds_headers = \
        list.h \
        ringbuffer.h \
        params.h \
	dictionary.h \
	jhash.h \
	x_tree.h

ds_sources = \
	list.c \
        ringbuffer.c \
	params.c \
	dictionary.c \
	jhash.c \
	x_tree.c

ds_tests = \
	params_test \
	dictionary-test \
	jhash-test

params_test_SOURCES = list.c params.c params_test.c
jhash_test_SOURCES = jhash.c jhash-test.c
dictionary_test_SOURCES = x_tree.c jhash.c dictionary.c dictionary-test.c

# HTTP handling

http_headers = \
        http-date.h \
        http-reqline.h \
        http-response.h \
        http-status.h

http_sources = \
        http-date.c \
        http-reqline.c \
        http-response.c \
        http-status.c

http_tests = \
	http-date_test \
	http-reqline_test

http_date_test_SOURCES = http-date.c http-date_test.c
http_reqline_test_SOURCES = http-reqline.c http-reqline_test.c

# Main
noinst_HEADERS = \
	$(ds_headers) \
	$(http_headers) \
        log.h \
        stream.h \
        flim.h \
	kongou.h \
        status.h \
        uiomux.h \
        tests.h

sighttpd_SOURCES = \
	$(ds_sources) \
	$(http_sources) \
	main.c \
        log.c \
        stream.c \
        flim.c \
	kongou.c \
        status.c \
        uiomux.c

sighttpd_LDFLAGS = $(PTHREAD_LIBS) $(RT_LIBS)

# Unit tests
test: check

TESTS = $(ds_tests) $(http_tests)

noinst_PROGRAMS = $(TESTS)

