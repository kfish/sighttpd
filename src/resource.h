#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "sighttpd.h"
#include "params.h"
#include "http-reqline.h"

typedef int (*ResourceCheck) (http_request * request, void * data);
typedef void (*ResourceHead) (http_request * request, params_t * request_headers,
		const char ** status_line, params_t ** response_headers, void * data);
typedef void (*ResourceBody) (int fd, http_request * request, params_t * request_headers, void * data);

struct resource {
	ResourceCheck check;
	ResourceHead head;
	ResourceBody body;
	void * data;
};

struct resource * resource_new (ResourceCheck check, ResourceHead head, ResourceBody body, void * data);

#endif /* __RESOURCE_H__ */
