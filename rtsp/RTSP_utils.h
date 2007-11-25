/*
 *  $Id: $
 *
 *  Copyright (C) 2007 by team@streaming.polito.it
 *
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

/**
 * @addtogroup RTSP
 * @{
 */

/** 
  * RTSP connection informations used by RTSP method functions
  */
typedef struct
{
    char object[255]; //!< path of the required media
    char server[255]; //!< local hostname
    char *address; //!< local address
    unsigned short port; //!< local port where the server is listening for requests

    description_format descr_format; //!< format of the media description
    char descr[MAX_DESCR_LENGTH]; //!< media description
} ConnectionInfo;

/**
 * RTSP Header and request parsing and validation functions
 * @defgroup rtsp_validation RTSP requests parsing and validation
 * @{
 */
RTSP_Error check_forbidden_path(ConnectionInfo * cinfo);
RTSP_Error validate_url(char *url, ConnectionInfo * cinfo);
RTSP_Error check_require_header(RTSP_buffer * rtsp);
RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer);
RTSP_Error get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo);
RTSP_Error get_cseq(RTSP_buffer * rtsp);
RTSP_Error get_session_description(ConnectionInfo * cinfo);
RTSP_Error get_session_id(RTSP_buffer * rtsp, unsigned long * session_id);
int max_connection();
/**
 * @}
 */

/**
 * RTSP Message generation and logging functions
 * @defgroup rtsp_msg_gen RTSP Message Generation
 * @{
 */
int send_redirect_3xx(RTSP_buffer *, char *);
int bwrite(char *buffer, unsigned short len, RTSP_buffer * rtsp);
void add_time_stamp(char *b, int crlf);
/**
 * @}
 */

char const *get_stat(int err);
void log_user_agent(RTSP_buffer * rtsp);
extern RTSP_Error const RTSP_Fatal_ErrAlloc;

/**
 * @}
 */
