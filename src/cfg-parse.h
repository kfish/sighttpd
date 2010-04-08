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

#ifndef __CFG_PARSE_H__
#define __CFG_PARSE_H__

/** \file
 *
 * A configuration file parser.
 */

typedef enum {
  COPA_STOP_ERR = -2,
  COPA_SYS_ERR = -1,
  COPA_OK = 0,
  COPA_STOP = 1,
  COPA_SKIP_SECTION = 2
} CopaStatus;

/**
 * Signature of a callback to be called when a [Section] is found.
 * \param name The name of the section
 * \param user_data A generic pointer you have provided earlier
 * \retval COPA_STOP_ERR Stop all parsing, return COPA_STOP_ERR to
 *         copa_read() or copa_read_fd()
 * \retval COPA_OK Continue and call the Assign callbacks for this section.
 * \retval COPA_STOP Stop all parsing, return COPA_STOP to copa_read() or
 *         copa_read_fd()
 * \retval COPA_SKIP_SECTION Skip this section, ie. don't call the Assign
 *         callback for this section. If this was the last section in the
 *         .ini file, COPA_OK will be returned to copa_read() or copa_read_fd()
 */
typedef CopaStatus (*CopaSection) (const char * name, void * user_data);

/**
 * Signature of a callback to be called when an assignment is found.
 * \param name The name of the variable
 * \param value The value of the variable
 * \param user_data A generic pointer you have provided earlier
 * \retval 0 Ignored anyway :)
 */
typedef CopaStatus (*CopaAssign) (const char * name, const char * value,
				  void * user_data);

/**
 * Read an .ini file
 * \param path The file to read
 * \param section A callback for Copa to call when a new [section] is found
 * \param section_user_data Arbitrary data you wish to pass to your
 *        \a section callback.
 * \param assign A callback for Copa to call when a new assignment is found
 * \param assign_user_data Arbitrary data you wish to pass to your
 *        \a assign callback.
 */
CopaStatus copa_read (char * path,
		      CopaSection section, void * section_user_data,
		      CopaAssign assign, void * assign_user_data);

/**
 * Read .ini data from a file descriptor
 * \param fd The file descriptor to read from
 * \param section A callback for Copa to call when a new [section] is found
 * \param section_user_data Arbitrary data you wish to pass to your
 *        \a section callback.
 * \param assign A callback for Copa to call when a new assignment is found
 * \param assign_user_data Arbitrary data you wish to pass to your
 *        \a assign callback.
 */
int copa_read_fd (int fd,
		  CopaSection section, void * section_user_data,
		  CopaAssign assign, void * assign_user_data);

#endif /* __CFG_PARSE_H__ */
