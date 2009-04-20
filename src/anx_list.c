/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if HAVE_CONFIG
#include "config.h"
#endif

#include "anx_compat.h"

#include <annodex/anx_list.h>

static AnxList *
anx_list_node_new (void * data)
{
  AnxList * l;

  l = (AnxList *) anx_malloc (sizeof (AnxList));
  l->prev = l->next = NULL;
  l->data = data;

  return l;
}

AnxList *
anx_list_new (void)
{
  return NULL;
}

AnxList *
anx_list_clone (AnxList * list)
{
  AnxList * l, * new_list;

  if (list == NULL) return NULL;
  new_list = anx_list_new ();

  for (l = list; l; l = l->next) {
    new_list = anx_list_append (new_list, l->data);
  }

  return new_list;
}

AnxList *
anx_list_clone_with (AnxList * list, AnxCloneFunc clone)
{
  AnxList * l, * new_list;
  void * new_data;

  if (list == NULL) return NULL;
  if (clone == NULL) return anx_list_clone (list);

  new_list = anx_list_new ();

  for (l = list; l; l = l->next) {
    new_data = clone (l->data);
    new_list = anx_list_append (new_list, new_data);
  }

  return new_list;
}


AnxList *
anx_list_tail (AnxList * list)
{
  AnxList * l;
  for (l = list; l; l = l->next)
    if (l->next == NULL) return l;
  return NULL;
}

AnxList *
anx_list_prepend (AnxList * list, void * data)
{
  AnxList * l = anx_list_node_new (data);

  if (list == NULL) return l;

  l->next = list;
  list->prev = l;

  return l;
}

AnxList *
anx_list_append (AnxList * list, void * data)
{
  AnxList * l = anx_list_node_new (data);
  AnxList * last;

  if (list == NULL) return l;

  last = anx_list_tail (list);
  if (last) last->next = l;
  l->prev = last;
  return list;
}

AnxList *
anx_list_add_before (AnxList * list, void * data, AnxList * node)
{
  AnxList * l, * p;

  if (list == NULL) return anx_list_node_new (data);
  if (node == NULL) return anx_list_append (list, data);
  if (node == list) return anx_list_prepend (list, data);

  l = anx_list_node_new (data);
  p = node->prev;

  l->prev = p;
  l->next = node;
  if (p) p->next = l;
  node->prev = l;

  return list;
}

AnxList *
anx_list_add_after (AnxList * list, void * data, AnxList * node)
{
  AnxList * l, * n;

  if (node == NULL) return anx_list_prepend (list, data);

  l = anx_list_node_new (data);
  n = node->next;

  l->prev = node;
  l->next = n;
  if (n) n->prev = l;
  node->next = l;

  return list;
}

AnxList *
anx_list_find (AnxList * list, void * data)
{
  AnxList * l;

  for (l = list; l; l = l->next)
    if (l->data == data) return l;

  return NULL;
}

AnxList *
anx_list_remove (AnxList * list, AnxList * node)
{
  if (node == NULL) return list;

  if (node->prev) node->prev->next = node->next;
  if (node->next) node->next->prev = node->prev;

  if (node == list) return list->next;
  else return list;
}

int
anx_list_length (AnxList * list)
{
  AnxList * l;
  int c = 0;

  for (l = list; l; l = l->next)
    c++;

  return c;
}

int
anx_list_is_empty (AnxList * list)
{
  return (list == NULL);
}

int
anx_list_is_singleton (AnxList * list)
{
  if (list == NULL) return 0;
  if (list->next == NULL) return 1;
  else return 0;
}

/*
 * anx_list_free_with (list, free_func)
 *
 * Step through list 'list', freeing each node using free_func(), and
 * also free the list structure itself.
 */
AnxList *
anx_list_free_with (AnxList * list, AnxFreeFunc free_func)
{
  AnxList * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    free_func (l->data);
    anx_free (l);
  }

  return NULL;
}

/*
 * anx_list_free (list)
 *
 * Free the list structure 'list', but not its nodes.
 */
AnxList *
anx_list_free (AnxList * list)
{
  AnxList * l, * ln;

  for (l = list; l; l = ln) {
    ln = l->next;
    anx_free (l);
  }

  return NULL;
}
