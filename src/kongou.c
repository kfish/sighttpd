#include <stdio.h>
#include <string.h>

#include "params.h"

#define MAX_FIELD 64

#define KONGOU_HEAD "<html><head><title>Kongou</title></head><body><h1>Kongou</h1>"
#define KONGOU_FOOT "</body></html>"

#define KONGOU_TEXT "kongou kongou kongou\n"

#define TTYSC 5

struct kongou_field {
        int no;
        char * name;
        int range_min;
        int range_max;
        int dflt;
};

struct kongou_control {
        struct kongou_field fields[MAX_FIELD];
};

struct kongou_field *
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

void
kongou_field_init (struct kongou_control * control, int no, char * name,
                   int range_min, int range_max, int dflt)
{
        struct kongou_field * field;


        field = &control->fields[no];

        field->no = no;
        field->name = name;
        field->range_min = range_min;
        field->range_max = range_max;
        field->dflt = dflt;
}

int
kongou_control_init (struct kongou_control * control)
{
  struct kongou_field * field;

  memset (control, 0, sizeof(struct kongou_control));

  kongou_field_init (control, 5, "EZOOM", 0x0000, 0x00e0, 0000);
  kongou_field_init (control, 6, "MIRROR", 0x0000, 0x0003, 0000);
  kongou_field_init (control, 7, "EFFECT", 0x0000, 0x0003, 0000);

  return 0;
}

params_t *
kongou_append_headers (params_t * response_headers)
{
        response_headers = params_append (response_headers, "Content-Type", "text/html");
}

struct handle_data {
        struct kongou_control * control;
        int fd;
};

int
kongou_handle_param (char * key, char * value, void * user_data)
{
        struct handle_data * h = (struct handle_data *)user_data;
        struct kongou_control * control = h->control;
        int fd = h->fd;
        struct kongou_field * field;
        char buf[1024], cmd[64];
        size_t n;
        int val;

        if (key != NULL) {
                field = kongou_get_field (control, key);
        }

        if (value != NULL)
                val = atoi(value);

        if (val < field->range_min || val > field->range_max) {
                n = snprintf (buf, 1024, "%s: Value 0x%04x out of range (0x%04x - 0x%04x)\n",
                              key, val, field->range_min, field->range_max);
        } else {
                n = snprintf (cmd, 64, "kgctrl set %d %d %d", TTYSC, field->no, val);
                n = snprintf (buf, 1024, "<li>Set %s to %d: <tt>%s</tt></li>\n", key, val, cmd);
        }

        write (fd, buf, n);

        return 0;
}

int
kongou_stream_body (int fd, char * path)
{
        char *q;
        char buf[1024], cmd[64];
        params_t * query;
        char * wb_p;
        int wb=0;
        size_t n;

        struct kongou_control control;
        struct handle_data h;

        kongou_control_init (&control);

        q = index (path, '?');

        n = snprintf (buf, 1024, KONGOU_HEAD);
        write (fd, buf, n);

        if (q == NULL) {
                n = snprintf (buf, 1024, "kongou!! %s\n", path);
                write (fd, buf, n);
        } else {
                q++;
                query = params_new_parse (q, strlen(q), PARAMS_QUERY);
 
                n = snprintf (buf, 1024, "<ul>");
                write (fd, buf, n);

                h.control = &control;
                h.fd = fd;
                params_foreach (query, kongou_handle_param, &h);

                n = snprintf (buf, 1024, "</ul>");
                write (fd, buf, n);
        }

        n = snprintf (buf, 1024, KONGOU_FOOT);
        write (fd, buf, n);
}

