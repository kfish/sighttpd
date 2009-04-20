/*
   Copyright (C) 2009 Conrad Parker
*/

#ifndef __PARAMS_H__
#define __PARAMS_H__

/** \file
 * Manipulation of name=value parameters, with parsing of URI query
 * strings and HTTP-style headers, and printing to these formats and
 * XML meta and param tags.
 *
 * - Field names are case-insensitive
 * - Field names are unique within an Params object. When parsing
 *   URI query strings or HTTP-style headers, if multiple entries
 *   for the same field name are found, the successive values are
 *   concatenated and separated by commas (as per RFC2616 sec 4.2)
 */

/**
 * A set of parameters.
 * - NULL is equivalent to an empty parameter set
 * - Create with params_new_parse() or params_clone(), or by
 *   adding a parameter to the empty set (NULL)
 * - The base value of an Params object is updated by calls to
 *   params_replace(), params_append(), params_remove(),
 *   params_merge() and params_clone(). Hence the return value
 *   from these functions \b must be assigned back to the params object.
 */
typedef void Params;

/**
 * Formatting styles for parsing and printing Params objects
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
} ParamStyle;

/**
 * Create a new AnxParms object by parsing text input of a given format
 * \param input The text to parse
 * \param len Length in bytes of \a input
 * \param style The formatting style of the text. Only
 *              PARAMS_QUERY and PARAMS_HEADERS are supported.
 * \returns A new Params object
 * \retval NULL no parameters found in input, or unsupported style
 */
Params * params_new_parse (char * input, size_t len, ParamStyle style);

/**
 * Print an AnxParms object with a given formatting style
 * \param buf The output buffer
 * \param n The maximum number of bytes to write
 * \param params The Params object
 * \param style The formatting style of the generated text.
 * \returns The number of characters printed, not including the trailing '\\0'
 *
 * <b>Truncation</b>:
 * - On systems conforming to ISO C99, snprint_params() returns the
 *   number of characters (not including the trailing '\\0') that would have
 *   been written to the output string if enough space had been available.
 * - On non-C99 systems, snprint_params() returns -1 on truncation.
 */
int params_snprint (char * buf, size_t n, Params * params,
			ParamStyle style);

/**
 * Retrieve a parameter from an Params object.
 * \param params An Params object
 * \param name The parameter name
 * \returns The parameter value
 * \retval NULL No such parameter
 */
char * params_get (Params * params, char * name);

/**
 * Add a parameter to an Params object.
 * If a parameter with the given \a name already exists in \a params,
 * the new \a value replaces the old one.
 * \param params An Params object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated Params object
 */
Params * params_replace (Params * params, char * name, char * value);

/**
 * Add a parameter to an Params object.
 * If a parameter with the given \a name already exists  in \a params,
 * the new \a value is appended to the old one, separated by a comma.
 * \param params An Params object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated Params object
 */
Params * params_append (Params * params, char * name, char * value);

/**
 * Remove a parameter from an Params object.
 * \param params An Params object
 * \param name The parameter name
 * \returns The updated Params object
 */
Params * params_remove (Params * params, char * name);

/**
 * Merge two Params objects. Copies of all parameters in \a src are
 * appended (as for params_append()) to \a dest.
 * \param dest The Params object into which new values are appended
 * \param src An Params object with the new values.
 * \returns An updated reference to \a dest.
 * \note \a src is not modified by params_merge().
 */
Params * params_merge (Params * dest, Params * src);

/**
 * Create a new Params object by cloning an existing one.
 * \param params An existing Params object.
 * \returns A new Params object with copied parameters. All names and
 *          values in \params are duplicated in the returned object.
 */
Params * params_clone (Params * params);

/**
 * Free an Params object
 * \param params An Params object
 * \returns NULL on success
 */
Params * params_free (Params * params);

#endif /* __PARAMS_H__ */
