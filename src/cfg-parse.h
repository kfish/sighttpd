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

#ifndef __XINI_H__
#define __XINI_H__

/** \file
 *
 * A .ini file parser.
 */

typedef enum {
  XINI_STOP_ERR = -2,
  XINI_SYS_ERR = -1,
  XINI_OK = 0,
  XINI_STOP = 1,
  XINI_SKIP_SECTION = 2
} XiniStatus;

/**
 * Signature of a callback to be called when a [Section] is found.
 * \param name The name of the section
 * \param user_data A generic pointer you have provided earlier
 * \retval XINI_STOP_ERR Stop all parsing, return XINI_STOP_ERR to
 *         xini_read() or xini_read_fd()
 * \retval XINI_OK Continue and call the Assign callbacks for this section.
 * \retval XINI_STOP Stop all parsing, return XINI_STOP to xini_read() or
 *         xini_read_fd()
 * \retval XINI_SKIP_SECTION Skip this section, ie. don't call the Assign
 *         callback for this section. If this was the last section in the
 *         .ini file, XINI_OK will be returned to xini_read() or xini_read_fd()
 */
typedef XiniStatus (*XiniSection) (const char * name, void * user_data);

/**
 * Signature of a callback to be called when an assignment is found.
 * \param name The name of the variable
 * \param value The value of the variable
 * \param user_data A generic pointer you have provided earlier
 * \retval 0 Ignored anyway :)
 */
typedef XiniStatus (*XiniAssign) (const char * name, const char * value,
				  void * user_data);

/**
 * Read an .ini file
 * \param path The file to read
 * \param section A callback for Xini to call when a new [section] is found
 * \param section_user_data Arbitrary data you wish to pass to your
 *        \a section callback.
 * \param assign A callback for Xini to call when a new assignment is found
 * \param assign_user_data Arbitrary data you wish to pass to your
 *        \a assign callback.
 */
XiniStatus xini_read (char * path,
		      XiniSection section, void * section_user_data,
		      XiniAssign assign, void * assign_user_data);

/**
 * Read .ini data from a file descriptor
 * \param fd The file descriptor to read from
 * \param section A callback for Xini to call when a new [section] is found
 * \param section_user_data Arbitrary data you wish to pass to your
 *        \a section callback.
 * \param assign A callback for Xini to call when a new assignment is found
 * \param assign_user_data Arbitrary data you wish to pass to your
 *        \a assign callback.
 */
int xini_read_fd (int fd,
		  XiniSection section, void * section_user_data,
		  XiniAssign assign, void * assign_user_data);

#endif /* __XINI_H__ */
