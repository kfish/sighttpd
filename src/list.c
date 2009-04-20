/*
   Copyright (C) 2009 Conrad Parker
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "list.h"

static List *
list_node_new (void * data)
{
  List * l;

  l = (List *) malloc (sizeof (List));
  l->prev = l->next = NULL;
  l->data = data;

  return l;
}

List *
list_new (void)
{
  return NULL;
}

List *
list_tail (List * list)
{
  List * l;
  for (l = list; l; l = l->next)
    if (l->next == NULL) return l;
  return NULL;
}

List *
list_prepend (List * list, void * data)
{
  List * l = list_node_new (data);

  if (list == NULL) return l;

  l->next = list;
  list->prev = l;

  return l;
}

List *
list_append (List * list, void * data)
{
  List * l = list_node_new (data);
  List * last;

  if (list == NULL) return l;

  last = list_tail (list);
  if (last) last->next = l;
  l->prev = last;
  return list;
}

List *
list_add_before (List * list, void * data, List * node)
{
  List * l, * p;

  if (list == NULL) return list_node_new (data);
  if (node == NULL) return list_append (list, data);
  if (node == list) return list_prepend (list, data);

  l = list_node_new (data);
  p = node->prev;

  l->prev = p;
  l->next = node;
  if (p) p->next = l;
  node->prev = l;

  return list;
}

List *
list_add_after (List * list, void * data, List * node)
{
  List * l, * n;

  if (node == NULL) return list_prepend (list, data);

  l = list_node_new (data);
  n = node->next;

  l->prev = node;
  l->next = n;
  if (n) n->prev = l;
  node->next = l;

  return list;
}

List *
list_find (List * list, void * data)
{
  List * l;

  for (l = list; l; l = l->next)
    if (l->data == data) return l;

  return NULL;
}

List *
list_remove (List * list, List * node)
{
  if (node == NULL) return list;

  if (node->prev) node->prev->next = node->next;
  if (node->next) node->next->prev = node->prev;

  if (node == list) return list->next;
  else return list;
}

int
list_length (List * list)
{
  List * l;
  int c = 0;

  for (l = list; l; l = l->next)
    c++;

  return c;
}

int
list_is_empty (List * list)
{
  return (list == NULL);
}

int
list_is_singleton (List * list)
{
  if (list == NULL) return 0;
  if (list->next == NULL) return 1;
  else return 0;
}

/*
 * list_free_with (list, free_func)
 *
 * Step through list 'list', freeing each node using free_func(), and
 * also free the list structure itself.
 */
List *
list_free_with (List * list, void * (*free_func)(void *))
{
  List * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    free_func (l->data);
    free (l);
  }

  return NULL;
}

/*
 * list_free (list)
 *
 * Free the list structure 'list', but not its nodes.
 */
List *
list_free (List * list)
{
  List * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    free (l);
  }

  return NULL;
}
