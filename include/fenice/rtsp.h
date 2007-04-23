/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#ifndef _RTSPH
#define _RTSPH

#include <time.h>
#include <config.h>
#include <fenice/utils.h>
#include <netembryo/wsocket.h>
#include <fenice/rtp.h>
#include <fenice/rtcp.h>
#include <fenice/sdp2.h>
#include <fenice/schedule.h>

#define RTSP_BUFFERSIZE 16384
#define RTSP_RESERVED 1024

    /* FIXME move rtsp states to an enum? */
#define INIT_STATE      0
#define READY_STATE     1
#define PLAY_STATE      2

#define RTSP_VER "RTSP/1.0"

#define RTSP_EL "\r\n"
#define RTSP_RTP_AVP "RTP/AVP"


typedef struct _RTSP_interleaved {
    Sock *rtp_local;
    Sock *rtcp_local;
    union {
        struct {
            uint16 rtp_ch;
            uint16 rtcp_ch;
        } tcp;
#ifdef HAVE_LIBSCTP
        struct {
            struct sctp_sndrcvinfo rtp;
            struct sctp_sndrcvinfo rtcp;
        } sctp;
#endif
    } proto;
    struct _RTSP_interleaved *next;
} RTSP_interleaved;

typedef struct _RTSP_session {
    int cur_state;
    int session_id;
    RTP_session *rtp_session;
    struct _RTSP_session *next;
    // mediathread resource
    Resource *resource;
} RTSP_session;

typedef struct _RTSP_buffer {
    Sock *sock;
    //unsigned int port;
    // Buffers      
    char in_buffer[RTSP_BUFFERSIZE];
    size_t in_size;
    char out_buffer[RTSP_BUFFERSIZE + MAX_DESCR_LENGTH];
    size_t out_size;
    /* vars for interleaved data:
     * interleaved pkts will be present only at the beginning of out_buffer.
     * this size is used to remenber how much data should be grouped in one
     * pkt with  MSG_MORE flag.
     * */
    RTSP_interleaved *interleaved;

    // Run-Time
    unsigned int rtsp_cseq;
    char descr[MAX_DESCR_LENGTH];
    RTSP_session *session_list;
    struct _RTSP_buffer *next;
} RTSP_buffer;

typedef enum {
       df_SDP_format = 0
} description_format;


/** 
 * RTSP high level functions, they maps to the rtsp methods
 * @defgroup rtsp_high RTSP high level functions
 * @{
 */

/**
 * All state transitions are made here except when the last stream packet
 * is sent during a PLAY.  That transition is located in stream_event().
 * @param rtsp the buffer containing the message to dispatch
 * @param method the id of the method to execute
 */
void RTSP_state_machine(RTSP_buffer * rtsp, int method_code);

/**
 * RTSP DESCRIBE method handler
 * It will not leave resources allocated.
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_describe(RTSP_buffer * rtsp);

/**
 * RTSP SETUP method handler
 * It will leave resources allocated, RTSP_teardown will clean up them
 * @param rtsp the buffer for which to handle the method
 * @param new_session where to save the newly allocated RTSP session
 * @return ERR_NOERROR
 */
int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session);

/**
 * RTSP PLAY method handler
 * It starts the streaming
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_play(RTSP_buffer * rtsp);

/**
 * RTSP PAUSE method handler
 * Puts the server on pause.
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_pause(RTSP_buffer * rtsp);

/**
 * RTSP TEARDOWN method handler
 * It severs the connection to the client and cleanups the resources
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_teardown(RTSP_buffer * rtsp);

/**
 * RTSP OPTIONS method handler
 * Queries the available methods and returns them.
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_options(RTSP_buffer * rtsp);

/**
 * Handles incoming RTSP message, validates them and then dispatches them 
 * with RTSP_state_machine
 * @param rtsp the buffer from where to read the message
 * @return ERR_NOERROR (can also mean RTSP_not_full if the message was not full)
 */
int RTSP_handler(RTSP_buffer * rtsp);

/**
 * @}
 */

#define RTSP_not_full 0
#define RTSP_method_rcvd 1
#define RTSP_interlvd_rcvd 2


/** 
 * RTSP low level functions, they handle message specific parsing and
 * communication.
 * @defgroup rtsp_low RTSP low level functions
 * @{
 */

/**
 * Recieves an RTSP message and puts it into the buffer
 * @param hdr_len where to save the header length
 * @param body_len where to save the message body length
 * @return -1 on ERROR
 * @return RTSP_not_full (0) if a full RTSP message is NOT present in the in_buffer yet.
 * @return RTSP_method_rcvd (1) if a full RTSP message is present in the in_buffer and is
 * ready to be handled.
 * @return RTSP_interlvd_rcvd (2) if a complete RTP/RTCP interleaved packet is present.  
 * terminate on really ugly cases.
 */
int RTSP_full_msg_rcvd(RTSP_buffer * rtsp, int *hdr_len, int *body_len);
#define RTSP_msg_len RTSP_full_msg_rcvd

/**
 * Removes the last message from the rtsp buffer
 * @param rtsp the buffer from which to discard the message
 */
void RTSP_discard_msg(RTSP_buffer * rtsp);

/** 
 * Removes a message from the input buffer
 * @param len the size of the message to remove
 * @param rtsp the buffer from which to remove the message
 */
void RTSP_remove_msg(int len, RTSP_buffer * rtsp);
//      void RTSP_msg_len(int *hdr_len,int *body_len,RTSP_buffer *rtsp);        

/**
 * Validates an incoming response message
 * @param status where to save the state specified in the message
 * @param msg where to save the message itself
 * @param rtsp the buffer containing the message
 * @return 1 if everything was fine
 * @return 0 if the parsed message wasn't a response message
 * @return ERR_GENERIC on generic error
 */
int RTSP_valid_response_msg(unsigned short *status, char *msg,
                RTSP_buffer * rtsp);

/** validates an rtsp message and returns its id
 * @param rtsp the buffer containing the message
 * @return the message id or -1 if something doesn't work in the request 
 */
int RTSP_validate_method(RTSP_buffer * rtsp);

/**
 * Initializes an RTSP buffer binding it to a socket
 * @param rtsp the buffer to inizialize
 * @param rtsp_sock the socket to link to the buffer
 */
void RTSP_initserver(RTSP_buffer * rtsp, Sock *rtsp_sock);

/**
 * Sends a reply message to the client
 * @param err the message code
 * @param addon the text to append to the message
 * @param rtsp the buffer where to write the output message
 * @return ERR_NOERROR on success
 * @return ERR_ALLOC if it was not possible to allocate enough space
 */
int send_reply(int err, char *addon, RTSP_buffer * rtsp);

/**
 * Sends the rtsp output buffer through the socket
 * @param rtsp the rtsp connection to flush through the socket
 * @return the size of data sent
 */
ssize_t RTSP_send(RTSP_buffer * rtsp);

#if 0 // To support multiple session per socket...
static inline RTSP_session *rtsp_session_from_id(RTSP_buffer *rtsp,
                                                 int session_id )
{
    RTSP_session *rtsp_sess;

    for (rtsp_sess = rtsp->session_list;
         rtsp_sess != NULL;
         rtsp_sess = rtsp_sess->next)
            if (rtsp_sess->session_id == session_id) break;

    return rtsp_sess;
}
static inline rtsp_session_list_free( RTSP_buffer *rtsp )
{
    RTSP_session *cur, *tmp;
    for (cur = rtsp->session_list;
         cur != NULL;) {
         tmp = cur;
         cur = cur->next;
         free(tmp);
    }
}
#endif 

/**
 * @}
 */

#endif
