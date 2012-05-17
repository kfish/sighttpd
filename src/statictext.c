/*
   Copyright (C) 2009 Conrad Parker
*/

#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"

#define DEFAULT_CONTENT_TYPE "text/plain"

#define x_strdup(s) ((s)?strdup((s)):(NULL))

struct statictext {
	char * path;
	char * text;
	char * ctype;
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

        r = params_append (r, "Content-Type", st->ctype);
        snprintf (length, 16, "%d", strlen (st->text));
        *response_headers = params_append (r, "Content-Length", length);
}

static void
statictext_body (int fd, http_request * request, params_t * request_headers, void * data)
{
	struct statictext * st = (struct statictext *)data;

        write (fd, st->text, strlen(st->text));
}

static void
statictext_delete (void * data)
{
	struct statictext * st = (struct statictext *)data;

	free (st->path);
	free (st->text);

	free (st);
}

struct resource *
statictext_resource (char * path, char * text, char * ctype)
{
	struct statictext * st;

	if ((st = calloc (1, sizeof(*st))) == NULL)
		return NULL;

	st->path = x_strdup(path);
	st->text = x_strdup(text);
	st->ctype = x_strdup(ctype);

	return resource_new (statictext_check, statictext_head, statictext_body, statictext_delete, st);
}

list_t *
statictext_resources (Dictionary * config)
{
	list_t * l;
	const char * path;
	const char * text;
	const char * ctype;

	l = list_new();

	path = dictionary_lookup (config, "Path");
	text = dictionary_lookup (config, "Text");
	ctype = dictionary_lookup (config, "Type");

	if (!ctype) ctype = DEFAULT_CONTENT_TYPE;

	if (path && text)
		l = list_append (l, statictext_resource (path, text, ctype));

	return l;
}

