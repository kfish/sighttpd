#ifndef __HTTP_REQLINE_H__
#define __HTTP_REQLINE_H__

/* http://www.ietf.org/rfc/rfc2616.txt */

typedef enum {
        HTTP_METHOD_OPTIONS,
        HTTP_METHOD_GET,
        HTTP_METHOD_HEAD,
        HTTP_METHOD_POST,
        HTTP_METHOD_PUT,
        HTTP_METHOD_DELETE,
        HTTP_METHOD_TRACE,
        HTTP_METHOD_CONNECT,
        HTTP_METHOD_EXTENSION
} http_method;

typedef enum {
        HTTP_VERSION_0_9,
        HTTP_VERSION_1_0,
        HTTP_VERSION_1_1
} http_version;

typedef struct {
        char * original_reqline;
        http_method method;
        http_version version;
        char * path;
} http_request;

size_t http_request_parse (char * s, size_t len, http_request * request);

#endif /* __HTTP_REQLINE_H__ */
