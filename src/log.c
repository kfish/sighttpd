#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "params.h"
#include "http-reqline.h"

#define ACCESS_LOG "/var/log/sighttpd/access.log"
#define ERROR_LOG "/var/log/sighttpd/error.log"

#define DATE_FMT "[%s] "
#define LOG_FMT DATE_FMT "\"%s\" 200 %s \"%s\"\r\n"

FILE * access_log=NULL, * error_log=NULL;

static int log_date (FILE * stream)
{
        char date[256];

        httpdate_snprint (date, 256, time(NULL));
        return fprintf (stream, DATE_FMT, date);
}

int log_open (void)
{
        access_log = fopen (ACCESS_LOG, "a");
        error_log = fopen (ERROR_LOG, "a");

        if (error_log != NULL) {
                log_date (error_log);
                fprintf (error_log, "Sighttpd/" VERSION " resuming normal operations\n");
                fflush (error_log);
        }
}

int log_close (void)
{
        if (access_log != NULL) {
                fflush (access_log);
                fclose (access_log);
        }
        if (error_log != NULL) {
                log_date (error_log);
                fprintf (error_log, "Sighttpd/" VERSION " shutting down\n");
                fflush (error_log);
                fclose (error_log);
        }
}

void log_access (http_request * request, Params * request_headers, Params * response_headers)
{
        char canon[1024];
        char * date, * user_agent, * content_length;

        /* Dump request headers to stdout */
        params_snprint (canon, 1024, request_headers, PARAMS_HEADERS);
        puts (canon);

        /* Apache-style logging */
        if ((date = params_get (response_headers, "Date")) == NULL)
            date = "";
        if ((user_agent = params_get (request_headers, "User-Agent")) == NULL)
            user_agent = "";
        if ((content_length = params_get (response_headers, "Content-Length")) == NULL)
            content_length = "";

        if (access_log != NULL) {
                fprintf (access_log, LOG_FMT, date, request->original_reqline,
                         content_length, user_agent);
                fflush (access_log);
        } else {
                fprintf (stderr, LOG_FMT, date, request->original_reqline,
                         content_length, user_agent);
        }
}

