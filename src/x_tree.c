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

#include <stdlib.h>

#include "config.h"

/* x_tree.c */

typedef struct _x_tree_t x_tree_t;
typedef struct _x_node_t x_node_t;

#define _X_TREE_PRIVATE
#include "x_tree.h"


#define x_malloc malloc
#define x_free free

#define x_new(T) (T *) x_malloc (sizeof(T))

struct _x_tree_t {
  x_cmp_t cmp;
  x_node_t * root;
};

struct _x_node_t {
  x_node_t * parent;
  x_node_t * left;
  x_node_t * right;
  void * data;
};

static x_node_t *
x_node_new (void * data)
{
  x_node_t * n = x_new (x_node_t);
  n->parent = n->left = n->right = NULL;
  n->data = data;
  return n;
}

x_tree_t *
x_tree_new (x_cmp_t cmp)
{
  x_tree_t * t = x_new (x_tree_t);
  t->cmp = cmp;
  t->root = NULL;
  return t;
}

#define ATTACH_LEFT(p,n) \
  (p)->left = (n);       \
  (n)->parent = (p);

#define ATTACH_RIGHT(p,n) \
  (p)->right = (n);       \
  (n)->parent = (p);


static __inline__ x_tree_t *
_x_tree_insert (x_tree_t * tree, x_node_t * node)
{
  x_node_t * m;

  if (tree == NULL) return NULL;

  m = tree->root;

  if (m == NULL) {
    tree->root = node;
    return tree;
  }

  while (m != NULL) {
    if (tree->cmp (node->data, m->data) < 0) {
      if (m->left != NULL) m = m->left;
      else {
	ATTACH_LEFT (m, node);
	m = NULL;
      }
    } else {
      if (m->right != NULL) m = m->right;
      else {
	ATTACH_RIGHT (m, node);
	m = NULL;
      }
    }
  }

  return tree;
}

x_tree_t *
x_tree_insert (x_tree_t * tree, void * data)
{
  x_node_t * node = x_node_new (data);
 
  if (tree == NULL) return NULL;

  return _x_tree_insert (tree, node);
}

static __inline__ x_node_t *
_x_tree_find (x_tree_t * tree, void * data)
{
  x_node_t * n;
  int ret;

  n = tree->root;
  while (n) {
    if ((ret = tree->cmp (data, n->data)) == 0) {
      return n;
    } else if (ret < 0) {
      n = n->left;
    } else {
      n = n->right;
    }
  }
  
  return NULL;
}

x_tree_t *
x_tree_remove (x_tree_t * tree, void * data)
{
  x_node_t * node = NULL;
  x_node_t * l, * r, * p, * lx;
  int was_left = 0;

  if (tree == NULL) return NULL;

  node = _x_tree_find (tree, data);
  if (node == NULL) return NULL;

  /* Cut node out of tree */

  if ((p = node->parent)) {
    if (node == p->left) {
      was_left = 1;
      p->left = NULL;
    }
    else
      p->right = NULL;
  }

  if ((l = node->left)) {
    l->parent = NULL;
  }

  if ((r = node->right)) {
    r->parent = NULL;
  }

  if (!p && !l && !r) {
    tree->root = NULL;
  } else if (!p && !l && r) {
    tree->root = r;
  } else if (!p && l && !r) {
    tree->root = l;
  } else if (p && !l && !r) {
    /* no further action */
  } else if (p && l && !r) {
    if (was_left) {
      ATTACH_LEFT (p, l);
    } else {
      ATTACH_RIGHT (p, l);
    }
  } else if (p && !l && r) {
    if (was_left) {
      ATTACH_LEFT (p, r);
    } else {
      ATTACH_RIGHT (p, r);
    }
  } else if (!p && l && r) {
    for (lx = l; lx->right; lx = lx->right);
    ATTACH_RIGHT (lx, r);
    tree->root = l;
  } else if (p && l && r) {
    for (lx = l; lx->right; lx = lx->right);
    ATTACH_RIGHT (lx, r);
    if (was_left) {
      ATTACH_LEFT (p, l);
    } else {
      ATTACH_RIGHT (p, l);
    }
  }

  x_free (node);

  return tree;
}

static __inline__ x_node_t *
_x_tree_first (x_tree_t * tree)
{
  x_node_t * m;
  for (m = tree->root; m && m->left != NULL; m = m->left);
  return m;
}

static __inline__ x_node_t *
_x_tree_last (x_tree_t * tree)
{
  x_node_t * m;
  for (m = tree->root; m && m->right != NULL; m = m->right);
  return m;
}

static __inline__ x_node_t *
_x_node_prev (x_tree_t * tree, x_node_t * node)
{
  x_node_t * m, * p;
  
  if (node->left == NULL) {
    p = node->parent;
    if (p && (p->right == node)) return p;
    else return NULL;
  }

  for (m = node->left; m->right != NULL; m = m->right);

  return m;
}

static __inline__ x_node_t *
_x_node_next (x_tree_t * tree, x_node_t * node)
{
  x_node_t * m, * p;
  
  if (node->right == NULL) {
    p = node->parent;
    if (p && (p->left == node)) return p;
    else return NULL;
  }

  for (m = node->right; m->left != NULL; m = m->left);

  return m;
}


/* Public functions */

x_node_t *
x_tree_find (x_tree_t * tree, void * data)
{
  if (tree == NULL) return NULL;
  return _x_tree_find (tree, data);
}

x_node_t *
x_tree_first (x_tree_t * tree)
{
  if (tree == NULL) return NULL;

  return _x_tree_first (tree);
}

x_node_t *
x_tree_last (x_tree_t * tree)
{
  if (tree == NULL) return NULL;

  return _x_tree_last (tree);
}

x_node_t *
x_node_prev (x_tree_t * tree, x_node_t * node)
{
  if (tree == NULL || node == NULL) return NULL;

  return _x_node_prev (tree, node);
}

x_node_t *
x_node_next (x_tree_t * tree, x_node_t * node)
{
  if (tree == NULL || node == NULL) return NULL;

  return _x_node_next (tree, node);
}

void *
x_node_data (x_tree_t * tree, x_node_t * node)
{
  if (tree == NULL || node == NULL) return NULL;

  return node->data;
}

static __inline__ x_node_t *
x_next_leaf (x_node_t * node)
{
  x_node_t * m = node;

  if (node == NULL) return NULL;

  while (1) {
    if (m->left) m = m->left;
    else if (m->right) m = m->right;
    else break;
  }

  return m;
}

x_tree_t *
x_tree_free_with (x_tree_t * tree, x_free_t free_func)
{
  x_node_t * n, * p;

  if (tree == NULL) return NULL;

  p = tree->root;

  while ((n = x_next_leaf (p))) {
    if ((p = n->parent)) {
      if (p->left == n) p->left = NULL;
      else p->right = NULL;
    }
    if (free_func)
      free_func (n->data);
    x_free (n);
  }

  x_free (tree);

  return NULL;
}

x_tree_t *
x_tree_free (x_tree_t * tree)
{
  x_tree_free_with (tree, NULL);
}
