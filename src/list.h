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
typedef struct _List List;

struct _List {
  List * prev;
  List * next;
  void * data;
};


#ifdef __cplusplus
extern "C" {
#endif

/** Create a new list
 * \return a new list
 */
List * list_new (void);

/**
 * Return the tail element of a list
 * \param list the list
 * \returns the tail element
 */
List * list_tail (List * list);

/**
 * Prepend a new node to a list containing given data
 * \param list the list
 * \param data the data element of the newly created node
 * \returns the new list head
 */
List * list_prepend (List * list, void * data);

/**
 * Append a new node to a list containing given data
 * \param list the list
 * \param data the data element of the newly created node
 * \returns the head of the list
 */
List * list_append (List * list, void * data);

/**
 * Add a new node containing given data before a given node
 * \param list the list
 * \param data the data element of the newly created node
 * \param node the node before which to add the newly created node
 * \returns the head of the list (which may have changed)
 */
List * list_add_before (List * list, void * data, List * node);

/**
 * Add a new node containing given data after a given node
 * \param list the list
 * \param data the data element of the newly created node
 * \param node the node after which to add the newly created node
 * \returns the head of the list
 */
List * list_add_after (List * list, void * data, List * node);

/**
 * Find the first node containing given data in a list
 * \param list the list
 * \param data the data element to find
 * \returns the first node containing given data, or NULL if it is not found
 */
List * list_find (List * list, void * data);

/**
 * Remove a node from a list
 * \param list the list
 * \param node the node to remove
 * \returns the head of the list (which may have changed)
 */
List * list_remove (List * list, List * node);

/**
 * Query the number of items in a list
 * \param list the list
 * \returns the number of nodes in the list
 */
int list_length (List * list);

/**
 * Query if a list is empty, ie. contains no items
 * \param list the list
 * \returns 1 if the list is empty, 0 otherwise
 */
int list_is_empty (List * list);

/**
 * Query if the list is singleton, ie. contains exactly one item
 * \param list the list
 * \returns 1 if the list is singleton, 0 otherwise
 */
int list_is_singleton (List * list);

/**
 * Free a list, using a given function to free each data element
 * \param list the list
 * \param free_func a function to free each data element
 * \returns NULL on success
 */
List * list_free_with (List * list, void (*free_func)(void *));

/**
 * Free a list, using free() to free each data element
 * \param list the list
 * \returns NULL on success
 */
List * list_free (List * list);

#ifdef __cplusplus
}
#endif

#endif /* __LIST_H__ */
