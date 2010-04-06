#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "sighttpd.h"
#include "params.h"
#include "http-reqline.h"

typedef int (*ResourceCheck) (struct sighttpd_child * schild, http_request * request);
typedef void (*ResourceHead) (struct sighttpd_child * schild, http_request * request,
		params_t * request_headers, const char ** status_line,
		params_t ** response_headers);
typedef void (*ResourceBody) (struct sighttpd_child * schild, http_request * request,
		params_t * request_headers);

struct resource {
	ResourceCheck check;
	ResourceHead head;
	ResourceBody body;
};

#endif /* __RESOURCE_H__ */
