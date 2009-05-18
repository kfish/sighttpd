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

#ifndef __X_TREE_H__
#define __X_TREE_H__

#ifndef _X_TREE_PRIVATE
typedef void x_tree_t;
typedef void x_node_t;
#endif

/* An x_cmp_t compares two scalars, returning:
 *  +ve, s1 > s2
 *    0, s1 == s2
 *  -ve, s1 < s2
 */
typedef int (*x_cmp_t) (void * s1, void * s2);

typedef void (*x_free_t) (void * ptr);

x_tree_t *
x_tree_new (x_cmp_t cmp);

x_tree_t *
x_tree_insert (x_tree_t * tree, void * data);

x_tree_t *
x_tree_remove (x_tree_t * tree, void * data);

x_node_t *
x_tree_find (x_tree_t * tree, void * data);

x_node_t *
x_tree_first (x_tree_t * tree);

x_node_t *
x_tree_last (x_tree_t * tree);

x_node_t *
x_node_prev (x_tree_t * tree, x_node_t * node);

x_node_t *
x_node_next (x_tree_t * tree, x_node_t * node);

void *
x_node_data (x_tree_t * tree, x_node_t * node);

x_tree_t *
x_tree_free (x_tree_t * tree);

x_tree_t *
x_tree_free_with (x_tree_t * tree, x_free_t free_func);

#endif /* __X_TREE_H__ */
