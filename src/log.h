#include "params.h"

int log_open (void);
int log_close (void);

void log_access (http_request * request, Params * request_headers, Params * response_headers);
