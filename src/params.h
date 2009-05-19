/*
   Copyright (C) 2009 Conrad Parker
*/

#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <stdlib.h>

/** \file
 * Manipulation of name=value parameters, with parsing of URI query
 * strings and HTTP-style headers, and printing to these formats and
 * XML meta and param tags.
 *
 * - Field names are case-insensitive
 * - Field names are unique within a params_t object. When parsing
 *   URI query strings or HTTP-style headers, if multiple entries
 *   for the same field name are found, the successive values are
 *   concatenated and separated by commas (as per RFC2616 sec 4.2)
 */

/**
 * A set of parameters.
 * - NULL is equivalent to an empty parameter set
 * - Create with params_new_parse() or params_clone(), or by
 *   adding a parameter to the empty set (NULL)
 * - The base value of a params_t object is updated by calls to
 *   params_replace(), params_append(), params_remove(),
 *   params_merge() and params_clone(). Hence the return value
 *   from these functions \b must be assigned back to the params object.
 */
typedef void params_t;

/**
 * Formatting styles for parsing and printing params_t objects
 */
typedef enum {
  /** URI query format, eg.
      <pre>fish=haddock&color=green
      </pre>
   */
  PARAMS_QUERY = 0,

  /** SMTP/HTTP/AnxData header format, eg.
      <pre>
      Fish: haddock\\r\\n
      Color: Green\\r\\n
      </pre>
  */
  PARAMS_HEADERS = 1,

  /** XHTML/CMML meta tag format, eg.:
      <pre>
      <meta name="fish" content="haddock"/>
      <meta name="color" content="green"/>
      </pre>
  */
  PARAMS_METATAGS = 1000,

  /** CMML param tag format, eg.:
      <pre>
      <param name="fish" content="haddock"/>
      <param name="color" content="green"/>
      </pre>
  */
  PARAMS_PARAMTAGS = 1001
} params_style;

/**
 * This is the signature of a callback function for params_foreach().
 * \param key The parameter name.
 * \param value The value of that parameter
 * \param user_data A generic pointer you have provided to params_foreach().
 * \returns 0 to continue, non-zero to instruct params_foreach() to stop.
 */
typedef int (*params_func) (char * key, char * value, void * user_data);

/**
 * Create a new params_t object by parsing text input of a given format
 * \param input The text to parse
 * \param len Length in bytes of \a input
 * \param style The formatting style of the text. Only
 *              PARAMS_QUERY and PARAMS_HEADERS are supported.
 * \returns A new params_t object
 * \retval NULL no parameters found in input, or unsupported style
 */
params_t * params_new_parse (char * input, size_t len, params_style style);

/**
 * Print a params_t object with a given formatting style
 * \param buf The output buffer
 * \param n The maximum number of bytes to write
 * \param params The params_t object
 * \param style The formatting style of the generated text.
 * \returns The number of characters printed, not including the trailing '\\0'
 *
 * <b>Truncation</b>:
 * - On systems conforming to ISO C99, snprint_params() returns the
 *   number of characters (not including the trailing '\\0') that would have
 *   been written to the output string if enough space had been available.
 * - On non-C99 systems, snprint_params() returns -1 on truncation.
 */
int params_snprint (char * buf, size_t n, params_t * params,
			params_style style);

/**
 * Retrieve a parameter from a params_t object.
 * \param params An params_t object
 * \param name The parameter name
 * \returns The parameter value
 * \retval NULL No such parameter
 */
char * params_get (params_t * params, char * name);

/**
 * Run a params_func on each element of a params_t object.
 * \param params A params_t object
 * \param func A params_func callback
 * \param user_data Arbitrary data you wish to pass to your callback
 * \retval 0 All parameters were processed.
 * \returns non-zero if any instance of the callback returned non-zero,
 * in which case params_foreach() is terminated and the last callback
 * result is returned.
 */
int params_foreach (params_t * params, params_func func, void * user_data);

/**
 * Add a parameter to a params_t object.
 * If a parameter with the given \a name already exists in \a params,
 * the new \a value replaces the old one.
 * \param params An params_t object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated params_t object
 */
params_t * params_replace (params_t * params, char * name, char * value);

/**
 * Add a parameter to a params_t object.
 * If a parameter with the given \a name already exists  in \a params,
 * the new \a value is appended to the old one, separated by a comma.
 * \param params An params_t object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated params_t object
 */
params_t * params_append (params_t * params, char * name, char * value);

/**
 * Remove a parameter from a params_t object.
 * \param params An params_t object
 * \param name The parameter name
 * \returns The updated params_t object
 */
params_t * params_remove (params_t * params, char * name);

/**
 * Merge two params_t objects. Copies of all parameters in \a src are
 * appended (as for params_append()) to \a dest.
 * \param dest The params_t object into which new values are appended
 * \param src An params_t object with the new values.
 * \returns An updated reference to \a dest.
 * \note \a src is not modified by params_merge().
 */
params_t * params_merge (params_t * dest, params_t * src);

/**
 * Create a new params_t object by cloning an existing one.
 * \param params An existing params_t object.
 * \returns A new params_t object with copied parameters. All names and
 *          values in \params are duplicated in the returned object.
 */
params_t * params_clone (params_t * params);

/**
 * Free a params_t object
 * \param params An params_t object
 * \returns NULL on success
 */
params_t * params_free (params_t * params);

#endif /* __PARAMS_H__ */
