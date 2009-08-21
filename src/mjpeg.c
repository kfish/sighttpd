#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "params.h"
#include "ringbuffer.h"

/*
 * Motion JPEG support -
 * 
 * The module expect that a series of jpeg comes in from stdin. At
 * each boundary the following headers must be inserted:
 *
 *    --++++++++
 *    Content-Type: image/jpeg
 *    Content-length: <size of jpeg file>
 *
 * mjpeg_test.sh is provided for testing purpose. Execute the following:
 *
 * % ./mjpeg_test.sh file1 file2 file3 ...
 * 
 * Then open 'http://localhost:3000/mjpeg/' with Firefox.
 */

/* #define DEBUG */

#define CONTENT_TYPE "multipart/x-mixed-replace; boundary=++++++++"

params_t *
mjpeg_append_headers (params_t * response_headers)
{
        char length[64];

        response_headers = params_append (response_headers, "Content-Type", CONTENT_TYPE);
}

