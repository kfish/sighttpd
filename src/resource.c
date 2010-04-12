/*
   Copyright (C) 2009 Conrad Parker
*/


#include "resource.h"

struct resource * resource_new (ResourceCheck check, ResourceHead head, ResourceBody body,
				ResourceDelete del, void * data)
{
	struct resource * r;

	if ((r = calloc (1, sizeof(struct resource))) == NULL)
		return NULL;

	r->check = check;
	r->head = head;
	r->body = body;
	r->del = del;
	r->data = data;

	return r;
}

void resource_delete (struct resource * resource)
{
	if (resource->del)
		resource->del(resource->data);
}
