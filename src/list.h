/*
   Copyright (C) 2009 Conrad Parker
*/

#ifndef __LIST_H__
#define __LIST_H__

/** \file
 * A doubly linked list
 */

/**
 * A doubly linked list
 */
typedef struct _list list_t;

struct _list {
  list_t * prev;
  list_t * next;
  void * data;
};


#ifdef __cplusplus
extern "C" {
#endif

/** Create a new list
 * \return a new list
 */
list_t * list_new (void);

/**
 * Return the tail element of a list
 * \param list the list
 * \returns the tail element
 */
list_t * list_tail (list_t * list);

/**
 * Prepend a new node to a list containing given data
 * \param list the list
 * \param data the data element of the newly created node
 * \returns the new list head
 */
list_t * list_prepend (list_t * list, void * data);

/**
 * Append a new node to a list containing given data
 * \param list the list
 * \param data the data element of the newly created node
 * \returns the head of the list
 */
list_t * list_append (list_t * list, void * data);

/**
 * Add a new node containing given data before a given node
 * \param list the list
 * \param data the data element of the newly created node
 * \param node the node before which to add the newly created node
 * \returns the head of the list (which may have changed)
 */
list_t * list_add_before (list_t * list, void * data, list_t * node);

/**
 * Add a new node containing given data after a given node
 * \param list the list
 * \param data the data element of the newly created node
 * \param node the node after which to add the newly created node
 * \returns the head of the list
 */
list_t * list_add_after (list_t * list, void * data, list_t * node);

/**
 * Find the first node containing given data in a list
 * \param list the list
 * \param data the data element to find
 * \returns the first node containing given data, or NULL if it is not found
 */
list_t * list_find (list_t * list, void * data);

/**
 * Remove a node from a list
 * \param list the list
 * \param node the node to remove
 * \returns the head of the list (which may have changed)
 */
list_t * list_remove (list_t * list, list_t * node);

/**
 * Join two lists.
 * \param l1 The first list
 * \param l2 The second list
 * \returns The joined list (l1++l2)
 */
list_t * list_join (list_t * l1, list_t * l2);

/**
 * Query the number of items in a list
 * \param list the list
 * \returns the number of nodes in the list
 */
int list_length (list_t * list);

/**
 * Query if a list is empty, ie. contains no items
 * \param list the list
 * \returns 1 if the list is empty, 0 otherwise
 */
int list_is_empty (list_t * list);

/**
 * Query if the list is singleton, ie. contains exactly one item
 * \param list the list
 * \returns 1 if the list is singleton, 0 otherwise
 */
int list_is_singleton (list_t * list);

/**
 * Free a list, using a given function to free each data element
 * \param list the list
 * \param free_func a function to free each data element
 * \returns NULL on success
 */
list_t * list_free_with (list_t * list, void * (*free_func)(void *));

/**
 * Free a list, using free() to free each data element
 * \param list the list
 * \returns NULL on success
 */
list_t * list_free (list_t * list);

#ifdef __cplusplus
}
#endif

#endif /* __LIST_H__ */
