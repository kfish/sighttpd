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

/*
 * cfg-parse.c
 *
 * A trivial parser for configuration files
 */

#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "cfg-parse.h"

/*#define DEBUG*/

#ifdef DEBUG
#include <stdio.h>
#else
#endif

#undef FALSE
#undef TRUE

#define FALSE (0)
#define TRUE (!FALSE)

#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))

#undef NUL
#define NUL '\0'

#define BUFFER_SIZE 8
#define INIT_SIZE 16

typedef struct _CopaParser CopaParser;

struct _CopaParser {
  char read_buffer[BUFFER_SIZE];
  CopaSection section;
  void * section_user_data;
  CopaAssign assign;
  void * assign_user_data;
  CopaStatus status;
  int fd;
  int active;
  char pushback;
  char numchar;
  int escape;
  int octal;
  int hex;
  size_t nread;
  size_t offset;
};

/* Characters */

#define BACKSLASH '\\'

/* Character classes */
#define X_NONE           0
#define X_NEWLINE     1<<0
#define X_WHITESPACE  1<<1
#define X_COMMENT     1<<2
#define X_LBRACKET    1<<3
#define X_RBRACKET    1<<4
#define X_DQUOTE      1<<5
#define X_SQUOTE      1<<6
#define X_EQUAL       1<<7

static int
copa_cin (char c, int char_class)
{
  if (char_class & X_NEWLINE)
    if (c == '\n' || c == '\r') return TRUE;

  if (char_class & X_WHITESPACE)
    if (isspace(c)) return TRUE;

  if (char_class & X_COMMENT)
    if (c == '#') return TRUE;

  if (char_class & X_LBRACKET)
    if (c == '<') return TRUE;

  if (char_class & X_RBRACKET)
    if (c == '>') return TRUE;

  if (char_class & X_DQUOTE)
    if (c == '"') return TRUE;

  if (char_class & X_SQUOTE)
    if (c == '\'') return TRUE;

  if (char_class & X_EQUAL)
    if (c == '=') return TRUE;

  return FALSE;
}

#ifdef DEBUG
/*
 * Dump read buffer
 */
static int
copa_dump (CopaParser * parser)
{
  char * c = parser->read_buffer;
  
  printf ("copa_dump: [%c] %d/%d >>>", parser->pushback,
	  parser->offset, parser->nread);
  switch (parser->nread) {
  case 8: putchar (*c++);
  case 7: putchar (*c++);
  case 6: putchar (*c++);
  case 5: putchar (*c++);
  case 4: putchar (*c++);
  case 3: putchar (*c++);
  case 2: putchar (*c++);
  case 1: putchar (*c++);
  default: putchar ('\n');
  }
}
#endif

/*
 * Refresh the read buffer
 */
static int
copa_refill (CopaParser * parser)
{
  parser->nread = read (parser->fd, parser->read_buffer, BUFFER_SIZE);
  if (parser->nread == -1) {
    /* XXX: error */
#ifdef DEBUG
    printf ("copa_next: nread == -1\n");
#endif
    parser->active = FALSE;
    return -1;
  } else if (parser->nread == 0) {
#ifdef DEBUG
    printf ("copa_next: nread == 0\n");
#endif
    parser->active = FALSE;
    return -1;
  }
  parser->offset = 0;

  return parser->nread;
}

static int
copa_octal (char c)
{
  int num;

  switch (c) {
  case '0': num = 0; break;
  case '1': num = 1; break;
  case '2': num = 2; break;
  case '3': num = 3; break;
  case '4': num = 4; break;
  case '5': num = 5; break;
  case '6': num = 6; break;
  case '7': num = 7; break;
  default: num = -1; break; /* non-octal */
  }

  return num;
}

static int
copa_hex (char c)
{
  int num;

  switch (c) {
  case '0': num = 0; break;
  case '1': num = 1; break;
  case '2': num = 2; break;
  case '3': num = 3; break;
  case '4': num = 4; break;
  case '5': num = 5; break;
  case '6': num = 6; break;
  case '7': num = 7; break;
  case '8': num = 8; break;
  case '9': num = 9; break;
  case 'a': case 'A': num = 10; break;
  case 'b': case 'B': num = 11; break;
  case 'c': case 'C': num = 12; break;
  case 'd': case 'D': num = 13; break;
  case 'e': case 'E': num = 14; break;
  case 'f': case 'F': num = 15; break;
  default: num = -1; break; /* non-hex */
  }

  return num;
}

/**
 * Retrieve the next character from the parser.
 * \param parser a CopaParser
 * \return the next character, or -1 on EOF or error.
 * \note parser->pushback will always be NUL after a call to this.
 */
static char
copa_next (CopaParser * parser)
{
  char c = NUL;
  int num;

#ifdef DEBUG
  copa_dump (parser);
#endif

  if (parser->pushback) {
    char tmp = parser->pushback;
#ifdef DEBUG
    printf ("copa_next: pushback %c [%02x]\n", tmp, tmp);
#endif
    parser->pushback = NUL;
    return tmp;
  }
  
  while (c == NUL) {
    if (parser->offset >= BUFFER_SIZE) {
#ifdef DEBUG
      printf ("copa_next: refill (%d/%d)\n", parser->offset, BUFFER_SIZE);
#endif
      if (copa_refill (parser) == -1) return -1;
    }
    
    c = parser->read_buffer[parser->offset++];
    
    if (parser->octal > 0) {
      parser->octal--;

      if ((num = copa_octal (c)) == -1) {
	/* End of octal input */
	parser->octal = 0;

	/* Simulate parse of new character, in
	 * preparation for next call. ie. prime
	 * pushback instead of returning c, then
	 * return stored octal value.
	 */
	if (c == BACKSLASH) {
	  parser->escape = TRUE;
	} else {
	  parser->pushback = c;
	}
	c = parser->numchar;
      } else {
	parser->numchar *= 8;
	parser->numchar += num;
	if (!parser->octal) c = parser->numchar;
	else c = NUL;
      }
    } else if (parser->hex > 0) {
      parser->hex--;

      if ((num = copa_hex (c)) == -1) {
	/* End of hex input */
	parser->hex = 0;

	/* Simulate parse of new character, in
	 * preparation for next call. ie. prime
	 * pushback instead of returning c, then
	 * return stored hex value.
	 */
	if (c == BACKSLASH){
	  parser->escape = TRUE;
	} else {
	  parser->pushback = c;
	}
	c = parser->numchar;
      } else {
	parser->numchar *= 16;
	parser->numchar += num;
	if (!parser->hex) c = parser->numchar;
	else c = NUL;
      }
    } else if (parser->escape) {
      parser->escape = FALSE;

      switch (c) {
      case 'a': c = '\a'; break;
      case 'b': c = '\b'; break;
      case 'f': c = '\f'; break;
      case 'n': c = '\n'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;
      case 'v': c = '\v'; break;
      case '\'': c = '\''; break;
      case '\\': c = '\\'; break;
      case '\?': c = '\?'; break;

      case '\n': case '\r': /* continuation */
	c = NUL;
	break;

      case 'x': /* hex */
	c = NUL;
	parser->hex = 2;
	parser->numchar = 0;
	break;

      default: /* try as octal */
	if ((num = copa_octal (c)) != -1) {
	  c = NUL;
	  parser->octal = 3;
	  parser->numchar = 0;
	  parser->numchar = num;
	} else {
	  /* Unknown escape sequence, pass through as identity:
	   * c = c;
	   */
	}
      }
    } else {
      if (c == BACKSLASH){
	parser->escape = TRUE;
	c = NUL;
      }
    }
  }

  return c;
}

static char
copa_peek (CopaParser * parser)
{
  char c;

  if (parser->pushback) {
#ifdef DEBUG
    printf ("copa_peek: pushback %c [%02x]\n",
	    parser->pushback, parser->pushback);
#endif
    return parser->pushback;
  }

  if ((c = copa_next (parser)) == -1) {
#ifdef DEBUG
    printf ("copa_peek: copa_next returned -1\n");
#endif
    return -1;
  }

#ifdef DEBUG
  printf ("copa_peek: %c [%02x]\n", c, c);
#endif

  parser->pushback = c;
  return c;
}

static void
copa_skip_over (CopaParser * parser, int char_class)
{
  char c;

  if (!parser->active) return;

  while ((c = copa_next (parser)) != -1) {
    if (!copa_cin (c, char_class)) {
      parser->pushback = c;
      return;
    }
  }
}

static void
copa_skip_whitespace (CopaParser * parser)
{
  copa_skip_over (parser, X_WHITESPACE);
}

static void
copa_skip_to (CopaParser * parser, int char_class)
{
  char c;

  if (!parser->active) return;

  while ((c = copa_next (parser)) != -1) {
    if (copa_cin (c, char_class)) {
      parser->pushback = c;
      return;
    }
  }

  return;
}

static char *
copa_slurp_to (CopaParser * parser, int good_end, int bad_end)
{
  char c, * ret, * new_ret;
  size_t len = 0, maxlen = INIT_SIZE;

  if (!parser->active) return NULL;

  ret = (char *) malloc (maxlen);
  ret[0] = NUL;

  while ((c = copa_next (parser)) != -1) {
    if (copa_cin(c, good_end)) {
      parser->pushback = c;
      return ret;
    } else if (copa_cin (c, bad_end)) {
      free (ret);
      parser->active = FALSE;
      parser->pushback = c;
      return NULL;
    }

    if (len >= maxlen-1) {
      maxlen *= 2;
      new_ret = realloc (ret, maxlen);
      if (new_ret == NULL) {
	parser->active = FALSE;
	return ret;
      }
      ret = new_ret;
    }

    ret[len++] = c;
    ret[len] = NUL;
  }

  return ret;
}

static int
copa_assert_and_pass (CopaParser * parser, int char_class)
{
  char c;

  if (!parser->active) return FALSE;

  c = copa_next (parser);

  if (!copa_cin (c, char_class)) {
    parser->active = FALSE;
    return FALSE;
  }

  return TRUE;
}

static char *
copa_slurp_quoted (CopaParser * parser)
{
  char c, * ret;
  int quote = X_DQUOTE; /* quote char to match on */

  if (!parser->active) return NULL;

  c = copa_next (parser);

  if (copa_cin (c, X_SQUOTE)) quote = X_SQUOTE;

  if ((ret = copa_slurp_to (parser, quote, X_NONE)) == NULL) {
    return NULL;
  } else if (!copa_assert_and_pass (parser, quote)) {
    free (ret);
    return NULL;
  }    

  return ret;
}

static char *
copa_slurp_lineval (CopaParser * parser)
{
  char * ret;

  if (!parser->active) return NULL;

  if ((ret = copa_slurp_to (parser, X_COMMENT | X_NEWLINE, X_NONE)) == NULL) {
    return NULL;
  }    

  return ret;
}

static int
copa_parse_assignment (CopaParser * parser)
{
  char c, * name, * value;

  if (!parser->active) return -1;

  name = copa_slurp_to (parser, X_WHITESPACE, X_COMMENT | X_NEWLINE);

#ifdef DEBUG
  printf ("copa_parse_assignment: got name (%s)\n", name);
#endif

  if (name == NULL) return -1;

  copa_skip_whitespace (parser);

  c = copa_peek (parser);
  if (copa_cin (c, X_DQUOTE | X_SQUOTE))
    value = copa_slurp_quoted (parser);
  else
    value = copa_slurp_lineval (parser);

  if (value == NULL) {
#ifdef DEBUG
    printf ("Got NULL quoted attribute value\n");
#endif
    goto err_free_name;
  }

#ifdef DEBUG
  printf ("copa_parse_assignment: %s = %s\n", name, value);
#endif

  /** Call callback */
  if (parser->status == COPA_OK && parser->assign) {
    parser->assign (name, value, parser->assign_user_data);
  }

  free (name);
  free (value);

  return 0;

 err_free_name:
  free (name);

  parser->active = FALSE;

  return -1;
}

static int
copa_parse_section (CopaParser * parser)
{
  char * section;

  if (!parser->active) return -1;

  if (!copa_assert_and_pass (parser, X_LBRACKET)) return -1;

  if ((section = copa_slurp_to (parser, X_RBRACKET,
				X_COMMENT | X_NEWLINE)) == NULL) {
#ifdef DEBUG
    printf ("copa_parse_section: FAILED\n");
#endif
    return -1;
  } else if (!copa_assert_and_pass (parser, X_RBRACKET)) {
#ifdef DEBUG
    printf ("copa_parse_section: did not get ]\n");
#endif
    free (section);
    return -1;
  }

  if (parser->section) {
    parser->status = parser->section (section, parser->section_user_data);
  }

  return 0;
}

static int
copa_parse_line (CopaParser * parser)
{
  char c;

  copa_skip_whitespace (parser);
  
  c = copa_peek (parser);
#ifdef DEBUG
  printf ("copa_parse_line: %c [%02x]\n", c, c);
#endif

  if (copa_cin (c, X_COMMENT)) {
#ifdef DEBUG
    printf ("copa_parse_line: got COMMENT\n");
#endif
    copa_skip_to (parser, X_NEWLINE);
  } else if (copa_cin (c, X_LBRACKET)) {
#ifdef DEBUG
    printf ("copa_parse_line: got SECTION\n");
#endif
    copa_parse_section (parser);
  } else {
#ifdef DEBUG
    printf ("copa_parse_line: attempt ASSIGNMENT\n");
#endif
    copa_parse_assignment (parser);
  }

  return 0;
}

int
copa_read_fd (int fd,
	      CopaSection section, void * section_user_data,
	      CopaAssign assign, void * assign_user_data)
{
  CopaParser parser;

  memset (parser.read_buffer, 0, BUFFER_SIZE);
  parser.section = section;
  parser.section_user_data = section_user_data;
  parser.assign = assign;
  parser.assign_user_data = assign_user_data;
  parser.status = COPA_OK;
  parser.fd = fd;
  parser.active = TRUE;
  parser.pushback = NUL;
  parser.escape = FALSE;
  parser.numchar = NUL;
  parser.octal = 0;
  parser.hex = 0;
  parser.nread = 0;
  parser.offset = 0;

  if (copa_refill (&parser) == -1) return COPA_SYS_ERR;

  while (parser.active) copa_parse_line (&parser);

  /* If the parse finished in a status of SKIP_SECTION, return OK */
  if (parser.status == COPA_SKIP_SECTION)
    parser.status = COPA_OK;

  return parser.status;
}

int
copa_read (char * path,
	   CopaSection section, void * section_user_data,
	   CopaAssign assign, void * assign_user_data)
{
  int fd;

  if ((fd = open (path, O_RDONLY)) == -1) return COPA_SYS_ERR;

  copa_read_fd (fd, section, section_user_data, assign, assign_user_data);

  return 0;
}
