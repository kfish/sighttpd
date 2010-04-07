/*
   Copyright (C) 2009 Conrad Parker
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "list.h"

static list_t *
list_node_new (void * data)
{
  list_t * l;

  l = (list_t *) malloc (sizeof (list_t));
  l->prev = l->next = NULL;
  l->data = data;

  return l;
}

list_t *
list_new (void)
{
  return NULL;
}

list_t *
list_tail (list_t * list)
{
  list_t * l;
  for (l = list; l; l = l->next)
    if (l->next == NULL) return l;
  return NULL;
}

list_t *
list_prepend (list_t * list, void * data)
{
  list_t * l = list_node_new (data);

  if (list == NULL) return l;

  l->next = list;
  list->prev = l;

  return l;
}

list_t *
list_append (list_t * list, void * data)
{
  list_t * l = list_node_new (data);
  list_t * last;

  if (list == NULL) return l;

  last = list_tail (list);
  if (last) last->next = l;
  l->prev = last;
  return list;
}

list_t *
list_add_before (list_t * list, void * data, list_t * node)
{
  list_t * l, * p;

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

list_t *
list_add_after (list_t * list, void * data, list_t * node)
{
  list_t * l, * n;

  if (node == NULL) return list_prepend (list, data);

  l = list_node_new (data);
  n = node->next;

  l->prev = node;
  l->next = n;
  if (n) n->prev = l;
  node->next = l;

  return list;
}

list_t *
list_find (list_t * list, void * data)
{
  list_t * l;

  for (l = list; l; l = l->next)
    if (l->data == data) return l;

  return NULL;
}

list_t *
list_remove (list_t * list, list_t * node)
{
  if (node == NULL) return list;

  if (node->prev) node->prev->next = node->next;
  if (node->next) node->next->prev = node->prev;

  if (node == list) return list->next;
  else return list;
}

list_t *
list_join (list_t * l1, list_t * l2)
{
  list_t * tail1 = list_tail(l1);

  if (tail1 == NULL) return l2;

  tail1->next = l2;
  if (l2 != NULL) l2->prev = tail1;

  return l1;
}

int
list_length (list_t * list)
{
  list_t * l;
  int c = 0;

  for (l = list; l; l = l->next)
    c++;

  return c;
}

int
list_is_empty (list_t * list)
{
  return (list == NULL);
}

int
list_is_singleton (list_t * list)
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
list_t *
list_free_with (list_t * list, void * (*free_func)(void *))
{
  list_t * l, * ln;

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
list_t *
list_free (list_t * list)
{
  list_t * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    free (l);
  }

  return NULL;
}
