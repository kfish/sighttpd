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

#ifndef __ANX_PARAMS_H__
#define __ANX_PARAMS_H__

/** \file
 * Manipulation of name=value parameters, with parsing of URI query
 * strings and HTTP-style headers, and printing to these formats and
 * XML meta and param tags.
 *
 * - Field names are case-insensitive
 * - Field names are unique within an AnxParams object. When parsing
 *   URI query strings or HTTP-style headers, if multiple entries
 *   for the same field name are found, the successive values are
 *   concatenated and separated by commas (as per RFC2616 sec 4.2)
 */

/**
 * A set of parameters.
 * - NULL is equivalent to an empty parameter set
 * - Create with anx_params_new_parse() or anx_params_clone(), or by
 *   adding a parameter to the empty set (NULL)
 * - The base value of an AnxParams object is updated by calls to
 *   anx_params_replace(), anx_params_append(), anx_params_remove(),
 *   anx_params_merge() and anx_params_clone(). Hence the return value
 *   from these functions \b must be assigned back to the params object.
 */
typedef void AnxParams;

/**
 * Formatting styles for parsing and printing AnxParams objects
 */
typedef enum {
  /** URI query format, eg.
      <pre>fish=haddock&color=green
      </pre>
   */
  ANX_PARAMS_QUERY = 0,

  /** SMTP/HTTP/AnxData header format, eg.
      <pre>
      Fish: haddock\\r\\n
      Color: Green\\r\\n
      </pre>
  */
  ANX_PARAMS_HEADERS = 1,

  /** XHTML/CMML meta tag format, eg.:
      <pre>
      <meta name="fish" content="haddock"/>
      <meta name="color" content="green"/>
      </pre>
  */
  ANX_PARAMS_METATAGS = 1000,

  /** CMML param tag format, eg.:
      <pre>
      <param name="fish" content="haddock"/>
      <param name="color" content="green"/>
      </pre>
  */
  ANX_PARAMS_PARAMTAGS = 1001
} AnxParamStyle;

/**
 * Create a new AnxParms object by parsing text input of a given format
 * \param input The text to parse
 * \param style The formatting style of the text. Only
 *              ANX_PARAMS_QUERY and ANX_PARAMS_HEADERS are supported.
 * \returns A new AnxParams object
 * \retval NULL no parameters found in input, or unsupported style
 */
AnxParams * anx_params_new_parse (char * input, AnxParamStyle style);

/**
 * Print an AnxParms object with a given formatting style
 * \param buf The output buffer
 * \param n The maximum number of bytes to write
 * \param params The AnxParams object
 * \param style The formatting style of the generated text.
 * \returns The number of characters printed, not including the trailing '\\0'
 *
 * <b>Truncation</b>:
 * - On systems conforming to ISO C99, anx_snprint_params() returns the
 *   number of characters (not including the trailing '\\0') that would have
 *   been written to the output string if enough space had been available.
 * - On non-C99 systems, anx_snprint_params() returns -1 on truncation.
 */
int anx_params_snprint (char * buf, size_t n, AnxParams * params,
			AnxParamStyle style);

/**
 * Retrieve a parameter from an AnxParams object.
 * \param params An AnxParams object
 * \param name The parameter name
 * \returns The parameter value
 * \retval NULL No such parameter
 */
char * anx_params_get (AnxParams * params, char * name);

/**
 * Add a parameter to an AnxParams object.
 * If a parameter with the given \a name already exists in \a params,
 * the new \a value replaces the old one.
 * \param params An AnxParams object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated AnxParams object
 */
AnxParams * anx_params_replace (AnxParams * params, char * name, char * value);

/**
 * Add a parameter to an AnxParams object.
 * If a parameter with the given \a name already exists  in \a params,
 * the new \a value is appended to the old one, separated by a comma.
 * \param params An AnxParams object
 * \param name The parameter name
 * \param value The new parameter value
 * \returns The updated AnxParams object
 */
AnxParams * anx_params_append (AnxParams * params, char * name, char * value);

/**
 * Remove a parameter from an AnxParams object.
 * \param params An AnxParams object
 * \param name The parameter name
 * \returns The updated AnxParams object
 */
AnxParams * anx_params_remove (AnxParams * params, char * name);

/**
 * Merge two AnxParams objects. Copies of all parameters in \a src are
 * appended (as for anx_params_append()) to \a dest.
 * \param dest The AnxParams object into which new values are appended
 * \param src An AnxParams object with the new values.
 * \returns An updated reference to \a dest.
 * \note \a src is not modified by anx_params_merge().
 */
AnxParams * anx_params_merge (AnxParams * dest, AnxParams * src);

/**
 * Create a new AnxParams object by cloning an existing one.
 * \param params An existing AnxParams object.
 * \returns A new AnxParams object with copied parameters. All names and
 *          values in \params are duplicated in the returned object.
 */
AnxParams * anx_params_clone (AnxParams * params);

/**
 * Free an AnxParams object
 * \param params An AnxParams object
 * \returns NULL on success
 */
AnxParams * anx_params_free (AnxParams * params);

#endif /* __ANX_PARAMS_H__ */
