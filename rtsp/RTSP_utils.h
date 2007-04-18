/*
 *  $Id: $
 *
 *  Copyright (C) 2007 by team@streaming.polito.it
 *
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libnms is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libnms; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#define MAX_REPLY_MESSAGE_LEN 256

/** 
  * RTSP connection informations used by RTSP method functions
  */
typedef struct
{
    char object[255]; //!< path of the required media
    char server[255]; //!< local hostname
    unsigned short port; //!< local port where the server is listening for requests

    description_format descr_format; //!< format of the media description
    char descr[MAX_DESCR_LENGTH]; //!< media description
} ConnectionInfo;

/**
  * RTSP reply message
  */
typedef struct
{
    int reply_code; //!< RTSP code representation of the message
    char reply_str[MAX_REPLY_MESSAGE_LEN]; //!< written representation of the message
} ReplyMessage;

/**
  * RTSP error description
  */
typedef struct
{
    ReplyMessage message; //!< RTSP standard error message
    int got_error; //!< can be: FALSE no error, TRUE generic error or have internal error id
} RTSP_Error;

extern RTSP_Error const RTSP_Ok;
extern RTSP_Error const RTSP_BadRequest;
extern RTSP_Error const RTSP_InternalServerError;
extern RTSP_Error const RTSP_Forbidden;
extern RTSP_Error const RTSP_OptionNotSupported;
extern RTSP_Error const RTSP_NotFound;
extern RTSP_Error const RTSP_SessionNotFound;
extern RTSP_Error const RTSP_Fatal_ErrAlloc;

void set_RTSP_Error(RTSP_Error * err, int reply_code, char * message);

RTSP_Error check_forbidden_path(ConnectionInfo * cinfo);
RTSP_Error validate_url(char *url, ConnectionInfo * cinfo);
RTSP_Error check_require_header(RTSP_buffer * rtsp);
RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer);
RTSP_Error get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo);
RTSP_Error get_cseq(RTSP_buffer * rtsp);
RTSP_Error get_session_description(ConnectionInfo * cinfo);
RTSP_Error get_session_id(RTSP_buffer * rtsp, long int * session_id);

void log_user_agent(RTSP_buffer * rtsp);

// Message functions
int send_redirect_3xx(RTSP_buffer *, char *);
int max_connection();
char const *get_stat(int err);
int bwrite(char *buffer, unsigned short len, RTSP_buffer * rtsp);
void add_time_stamp(char *b, int crlf);
