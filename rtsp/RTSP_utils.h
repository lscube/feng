/* *
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/** @file RTSP_utils.h
 * @brief Contains definitions of various RTSP utility structures and functions
 *
 * Contains definition of RTSP structures and constants for error management,
 * definition of various RTSP requests validation functions and various
 * internal functions
 */

#include <fenice/rtsp.h>
#include <netembryo/rtsp_errors.h>
#include <netembryo/url.h>

RTSP_Error rtsp_extract_validate_url(RTSP_buffer *rtsp, Url *url);;
RTSP_Error check_require_header(RTSP_buffer * rtsp);
RTSP_Error get_cseq(RTSP_buffer * rtsp);
RTSP_Error get_session_id(RTSP_buffer * rtsp, guint64 * session_id);

int send_redirect_3xx(RTSP_buffer *, char *);
int bwrite(char *buffer, size_t len, RTSP_buffer * rtsp);
GString *rtsp_generate_response(guint code, guint cseq);
GString *rtsp_generate_ok_response(guint cseq, guint64 session);

void log_user_agent(RTSP_buffer * rtsp);
extern RTSP_Error const RTSP_Fatal_ErrAlloc;
