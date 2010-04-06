#include <stdio.h>
#include <string.h>

#include "http-reqline.h"
#include "http-status.h"
#include "params.h"
#include "resource.h"
#include "shell.h"

#define MAX_FIELD 128

#define MAX_NAME 32

#define KONGOU_HEAD "<html><head><title>Kongou</title></head><body><h1>Kongou</h1>"
#define KONGOU_FOOT "</body></html>"

#define TTYSC 5

struct kongou_field {
        char name[MAX_NAME];
        int no;
        int range_min;
        int range_max;
        int dflt;
        int value;
};

struct kongou_control {
        struct kongou_field fields[MAX_FIELD];
};

static struct kongou_field *
kongou_get_field (struct kongou_control * control, char * fieldname)
{
  char * thisname;
  int i;

  for (i=0; i<MAX_FIELD; i++) {
    thisname = control->fields[i].name;
    if (thisname != NULL && !strcmp (fieldname, thisname)) {
      return &control->fields[i];
    }
  }

  return NULL;
}

static void
kongou_field_init (struct kongou_control * control, int no, char * name,
                   int range_min, int range_max, int dflt)
{
        struct kongou_field * field;

        field = &control->fields[no];

        field->no = no;
        strncpy (field->name, name, MAX_NAME);
        field->range_min = range_min;
        field->range_max = range_max;
        field->dflt = dflt;
}

static void
kongou_field_got (struct kongou_control * control, int no, int value)
{
        struct kongou_field * field;

        field = &control->fields[no];
        field->value = value;
}

static void
kongou_control_get_all (struct kongou_control * control)
{
        char cmd[64], buf[8192], * line, * prev_line;
        int n;
        int no, value;
        char name[64];

        snprintf (cmd, 64, "kgctrl get-all %d", TTYSC);
        n = shell_copy (buf, 8192, cmd);
        if (n == 0) return;

        prev_line = buf;
        while (*prev_line && (line = strchr (prev_line, '\n')) != NULL) {
                *line = '\0';
                if ((n = sscanf (prev_line, "%i %i", &no, &value)) == 2) {
                        kongou_field_got (control, no, value);
                }
                prev_line = line+1;
        }

        return;
}

static int
kongou_control_init_short (struct kongou_control * control)
{
        kongou_field_init (control, 5, "EZOOM", 0x0000, 0x00e0, 0000);
        kongou_field_init (control, 6, "MIRROR", 0x0000, 0x0003, 0000);
        kongou_field_init (control, 7, "EFFECT", 0x0000, 0x0003, 0000);

        return 0;
}

static int
kongou_control_init (struct kongou_control * control, int full)
{
        char buf[8192], * line, * prev_line;
        int n;
        int no, plen, range_min, range_max, dflt;
        char name[64];

        memset (control, 0, sizeof(struct kongou_control));

        kongou_control_get_all (control);

        if (!full)
                return kongou_control_init_short (control);

        n = shell_copy (buf, 8192, "kgctrl info");
        if (n == 0)
                return kongou_control_init_short (control);

        prev_line = buf;
        while (*prev_line && (line = strchr (prev_line, '\n')) != NULL) {
                *line = '\0';
                if ((n = sscanf (prev_line, "%i %s %i %i - %i %i",
                                 &no, name, &plen, &range_min, &range_max, &dflt)) == 6) {
                        kongou_field_init (control, no, name, range_min, range_max, dflt);
                }
                prev_line = line+1;
        }

        return 0;
}

static int
kongou_check (http_request * request, void * data)
{
        return !strncmp (request->path, "/kongou", 7);
}

static params_t *
kongou_append_headers (params_t * response_headers)
{
        response_headers = params_append (response_headers, "Content-Type", "text/html");
}

static void
kongou_head (http_request * request, params_t * request_headers, const char ** status_line,
		params_t ** response_headers, void * data)
{
        *status_line = http_status_line (HTTP_STATUS_OK);
        *response_headers = kongou_append_headers (*response_headers);
}

static int
kongou_field_entries (int fd, struct kongou_control * control)
{
  struct kongou_field * field;
  char buf[1024];
  size_t n;
  int i;

  n = snprintf (buf, 1024, "<form action=\"/kongou.html\" method=\"GET\">\n<table>\n");
  write (fd, buf, n);

  for (i=0; i<MAX_FIELD; i++) {
    field = &control->fields[i];

    if (*field->name != '\0') {
        n = snprintf (buf, 1024, "<tr><th>%s</th><td><input name=\"%s\" value=\"0x%04x\"/></td></tr>\n",
                      field->name, field->name, field->value);
        write (fd, buf, n);
    }
  }

  n = snprintf (buf, 1024, "</table><input type=\"submit\" value=\"Set\"></form>\n");
  write (fd, buf, n);

  return 0;
}

struct handle_data {
        struct kongou_control * control;
        int fd;
};

static int
kongou_set_param (char * key, char * value, void * user_data)
{
        struct handle_data * h = (struct handle_data *)user_data;
        struct kongou_control * control = h->control;
        int fd = h->fd;
        struct kongou_field * field = NULL;
        char buf[1024], cmd[64];
        size_t n;
        int val, ret;

        if (key != NULL) {
                field = kongou_get_field (control, key);
        }

        /* Ignore unknown or unset fields */
        if (field == NULL || value == NULL)
                return 0;

        val = atoi(value);

        /* Check if the value is unchanged */
        if (val == field->value) {
                return 0;
        }

        if (val < field->range_min || val > field->range_max) {
                n = snprintf (buf, 1024, "<li>%s: Value 0x%04x out of range (0x%04x - 0x%04x)\n</li>",
                              key, val, field->range_min, field->range_max);
                write (fd, buf, n);
        } else {
                n = snprintf (cmd, 64, "kgctrl set %d %d %d", TTYSC, field->no, val);
                n = snprintf (buf, 1024, "<li>Set %s to %d: <tt>%s</tt>", key, val, cmd);
                write (fd, buf, n);
#ifndef DEBUG
                ret = system (cmd);
                if (ret == -1) {
                        n = snprintf (buf, 1024, ": ERROR");
                } else {
                        n = snprintf (buf, 1024, ": OK");
                }
                write (fd, buf, n);
#endif
                n = snprintf (buf, 1024, "</li>\n");
                write (fd, buf, n);
        }


        return 0;
}

static void
kongou_body (int fd, http_request * request, params_t * request_headers, void * data)
{
        char *q;
        char buf[1024], cmd[64];
        params_t * query;
        char * wb_p;
        int wb=0;
        size_t n;
        int full;

        struct kongou_control control;
        struct handle_data h;

	char * path = request->path;

        full = (strstr (path, "full") != NULL);
        kongou_control_init (&control, full);

        q = index (path, '?');

        n = snprintf (buf, 1024, KONGOU_HEAD);
        write (fd, buf, n);

        h.control = &control;
        h.fd = fd;

        if (q != NULL) {
                q++;
                query = params_new_parse (q, strlen(q), PARAMS_QUERY);
 
                n = snprintf (buf, 1024, "<ul>");
                write (fd, buf, n);

                params_foreach (query, kongou_set_param, &h);

                n = snprintf (buf, 1024, "</ul><hr/>");
                write (fd, buf, n);
        }

        kongou_field_entries (fd, &control);

        n = snprintf (buf, 1024, KONGOU_FOOT);
        write (fd, buf, n);
}

struct resource *
kongou_resource (void)
{
	return resource_new (kongou_check, kongou_head, kongou_body, NULL);
}
