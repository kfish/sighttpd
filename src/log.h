#include "params.h"

int log_open (void);
int log_close (void);

void log_access (http_request * request, params_t * request_headers, params_t * response_headers);
