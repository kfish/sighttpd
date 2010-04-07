#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"

struct statictext {
	char * path;
	char * text;
};

static int
statictext_check (http_request * request, void * data)
{
	struct statictext * st = (struct statictext *)data;

        return !strncmp (request->path, st->path, strlen(st->path));
}

static void
statictext_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
	struct statictext * st = (struct statictext *)data;
	params_t * r = *response_headers;
        char length[16];

        *status_line = http_status_line (HTTP_STATUS_OK);

        r = params_append (r, "Content-Type", "text/plain");
        snprintf (length, 16, "%d", strlen (st->text));
        *response_headers = params_append (r, "Content-Length", length);
}

static void
statictext_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct statictext * st = (struct statictext *)data;

        write (fd, st->text, strlen(st->text));
}

struct resource *
statictext_resource (char * path, char * text)
{
	struct statictext * st;

	if ((st = calloc (1, sizeof(*st))) == NULL)
		return NULL;

	st->path = path;
	st->text = text;

	return resource_new (statictext_check, statictext_head, statictext_body, st);
}

list_t *
statictext_resources (Dictionary * config)
{
	list_t * l;
	const char * path;
	const char * text;

	l = list_new();

	path = dictionary_lookup (config, "StaticTextPath");
	text = dictionary_lookup (config, "StaticText");

	if (path && text)
		l = list_append (l, statictext_resource (path, text));

	return l;
}

