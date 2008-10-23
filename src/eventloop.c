/* * 
 *  This file is part of Feng
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
 * */

/**
 * @file eventloop.c
 * Network main loop
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

#include "eventloop.h"
#include <fenice/utils.h>

#include <fenice/schedule.h>

static GPtrArray *listen_socks;        //!< listen sockets
static fd_set rset;
static fd_set wset;
static int max_fd;
static GSList *clients; // of type RTSP_buffer

/**
 * Bind to the defined listening port
 */

int feng_bind_port(char *host, char *port, specific_config *s)
{
    int is_sctp = s->is_sctp;
    Sock *sock;

    if (is_sctp)
        sock = Sock_bind(host, port, NULL, SCTP, NULL);
    else
        sock = Sock_bind(host, port, NULL, TCP, NULL);
    if(!sock) {
        fnc_log(FNC_LOG_ERR,"Sock_bind() error for port %s.", port);
        fprintf(stderr,
                "[fatal] Sock_bind() error in main() for port %s.\n",
                port);
        return 1;
    }

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    g_ptr_array_add(listen_socks, sock);

    if(Sock_listen(sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }

    return 0;
}

static inline
int rtsp_sock_read(Sock *sock, int *stream, char *buffer, int size)
{
    int n;
#ifdef HAVE_LIBSCTP
    struct sctp_sndrcvinfo sctp_info;
    if (Sock_type(sock) == SCTP) {
        memset(&sctp_info, 0, sizeof(sctp_info));
        n = Sock_read(sock, buffer, size, &sctp_info, 0);
        *stream = sctp_info.sinfo_stream;
        fnc_log(FNC_LOG_DEBUG,
            "Sock_read() received %d bytes from sctp stream %d\n", n, stream);
    } else    // RTSP protocol is TCP
#endif    // HAVE_LIBSCTP

    n = Sock_read(sock, buffer, size, NULL, 0);

    return n;
}

#ifdef HAVE_LIBSCTP
#   define Sock_interleaved(a) \
     (Sock_type(a) == TCP || Sock_type(a) == SCTP)
#else
#   define Sock_interleaved(a) \
     (Sock_type(a) == TCP)
#endif

#ifdef HAVE_LIBSCTP
static gboolean find_sctp_interleaved(gconstpointer value, gconstpointer target)
{
  RTSP_interleaved *i = (RTSP_interleaved *)value;
  gint m = GPOINTER_TO_INT(target);

  return (i->proto.sctp.rtp.sinfo_stream == m || i->proto.sctp.rtcp.sinfo_stream == m);
}
#endif

static void interleaved_read(gpointer element, gpointer user_data)
{
  RTSP_buffer *rtsp = (RTSP_buffer *)user_data;
  RTSP_interleaved *intlvd = (RTSP_interleaved *)element;

  char buffer[RTSP_BUFFERSIZE + 1];    /* +1 to control the final '\0' */
  int n;
#ifdef HAVE_LIBSCTP
    struct sctp_sndrcvinfo sctp_info;
#endif

  if ( FD_ISSET(Sock_fd(intlvd->rtcp_local), &rset) ) {
    if ( (n = Sock_read(intlvd->rtcp_local, buffer, RTSP_BUFFERSIZE, NULL, 0)) < 0) {
      fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
      return;
    }
    switch (Sock_type(rtsp->sock)){
    case TCP: {
      uint16_t ne_n = htons((uint16_t)n);
      GString *str = g_string_sized_new(n+4);
      g_string_append_c(str, '$');
      g_string_append_c(str, (unsigned char)intlvd->proto.tcp.rtcp_ch);
      g_string_append_len(str, &ne_n, 2);
      g_string_append_len(str, &buffer, n);
      
      g_async_queue_push(rtsp->out_queue, str);
      if ( (n = RTSP_send(rtsp)) < 0) {
	send_reply(500, NULL, rtsp);
	return;// internal server error
      }
    }
    break;
#ifdef HAVE_LIBSCTP
    case SCTP:
      memcpy(&sctp_info, &(intlvd->proto.sctp.rtcp), sizeof(struct sctp_sndrcvinfo));
      Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR);
      break;
#endif
    default:
      break;
    }
  }
  if ( FD_ISSET(Sock_fd(intlvd->rtp_local), &rset) ) {
    if ( (n = Sock_read(intlvd->rtp_local, buffer,
                        RTSP_BUFFERSIZE, NULL, 0)) < 0) {
      // if ( (n = read(intlvd->rtp_fd, intlvd->out_buffer, sizeof(intlvd->out_buffer))) < 0) {
      fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
      return;
    }
    switch (Sock_type(rtsp->sock)){
    case TCP: {
      uint16_t ne_n = htons((uint16_t)n);
      GString *str = g_string_sized_new(n+4);
      g_string_append_c(str, '$');
      g_string_append_c(str, (unsigned char)intlvd->proto.tcp.rtcp_ch);
      g_string_append_len(str, &ne_n, 2);
      g_string_append_len(str, &buffer, n);
      
      g_async_queue_push(rtsp->out_queue, str);
      if ( (n = RTSP_send(rtsp)) < 0) {
	send_reply(500, NULL, rtsp);
	return;// internal server error
      }
    }
#ifdef HAVE_LIBSCTP
    case SCTP:
      memcpy(&sctp_info, &(intlvd->proto.sctp.rtp),
	     sizeof(struct sctp_sndrcvinfo));
      Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR);
      break;
#endif
    default:
      break;
    }
  }
}

static void rtp_session_read(gpointer element, gpointer user_data)
{
  RTP_session *p = (RTP_session*)element;
  
  if ( (p->transport.rtcp_sock) &&
       FD_ISSET(Sock_fd(p->transport.rtcp_sock), &rset)) {
    // There are RTCP packets to read in
    if (RTP_recv(p, rtcp_proto) < 0) {
      fnc_log(FNC_LOG_VERBOSE,
	      "Input RTCP packet Lost\n");
    } else {
      RTCP_recv_packet(p);
    }
    fnc_log(FNC_LOG_VERBOSE, "IN RTCP\n");
  }
}

static int rtsp_server(RTSP_buffer * rtsp)
{
    char buffer[RTSP_BUFFERSIZE + 1];    /* +1 to control the final '\0' */
    int n;
    int res;
    gint m = 0;

    if (rtsp == NULL) {
        return ERR_NOERROR;
    }
    if (FD_ISSET(Sock_fd(rtsp->sock), &wset)) { // first of all: there is some data to send?
        if ( (n = RTSP_send(rtsp)) < 0) {
//            send_reply(500, NULL, rtsp); FIXME
            return ERR_GENERIC;// internal server error
        }
    }
    if (FD_ISSET(Sock_fd(rtsp->sock), &rset)) {
        // There are RTSP or RTCP packets to read in
        memset(buffer, 0, sizeof(buffer));
        n = rtsp_sock_read(rtsp->sock, &m, buffer, sizeof(buffer) - 1);
        if (n == 0) {
            return ERR_CONNECTION_CLOSE;
        }
        if (n < 0) {
            fnc_log(FNC_LOG_DEBUG,
                "Sock_read() error in rtsp_server()\n");
            send_reply(500, NULL, rtsp);
            return ERR_GENERIC;
        }
        if (Sock_interleaved(rtsp->sock) && m == 0) {
            if (rtsp->in_size + n > RTSP_BUFFERSIZE) {
                fnc_log(FNC_LOG_DEBUG,
                    "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
                send_reply(500, NULL, rtsp);
                return ERR_GENERIC;    //errore da comunicare
            }
            memcpy(&(rtsp->in_buffer[rtsp->in_size]), buffer, n);
            rtsp->in_size += n;
            if ((res = RTSP_handler(rtsp)) == ERR_GENERIC) {
                fnc_log(FNC_LOG_ERR,
                    "Invalid input message.\n");
                return ERR_GENERIC;
            }
        } else {    /* if (rtsp->proto == SCTP && m != 0) */
#ifdef HAVE_LIBSCTP
	  RTSP_interleaved *intlvd = g_slist_find_custom(rtsp->interleaved, GINT_TO_POINTER(m), find_sctp_interleaved)->data;

	  if (intlvd) {
	    if (m == intlvd->proto.sctp.rtcp.sinfo_stream) {
	      Sock_write(intlvd->rtcp_local, buffer, n, NULL, 0);
	    } else {    // RTP pkt arrived: do nothing...
	      fnc_log(FNC_LOG_DEBUG,
		      "Interleaved RTP packet arrived from stream %d.\n",
		      m);
	    }
	  } else {
	    fnc_log(FNC_LOG_DEBUG,
                    "Packet arrived from unknown stream (%d)... ignoring.\n",
                    m);
	  }
#endif    // HAVE_LIBSCTP
        }
    }

    g_slist_foreach(rtsp->interleaved, interleaved_read, rtsp);

    if ( rtsp->session != NULL )
      g_slist_foreach(rtsp->session->rtp_sessions, rtp_session_read, NULL);

    return ERR_NOERROR;
}

static void interleaved_set_fds(gpointer element, gpointer user_data)
{
  RTSP_interleaved *intlvd = (RTSP_interleaved *)element;

  if (intlvd->rtp_local) {
    FD_SET(Sock_fd(intlvd->rtp_local), &rset);
    max_fd = MAX(max_fd, Sock_fd(intlvd->rtp_local));
  }
  if (intlvd->rtcp_local) {
    FD_SET(Sock_fd(intlvd->rtcp_local), &rset);
    max_fd = MAX(max_fd, Sock_fd(intlvd->rtcp_local));
  }
}

static void rtp_session_set_fd(gpointer element, gpointer user_data)
{
  RTP_session *p = (RTP_session*)element;
  RTSP_session *q = (RTSP_session*)user_data;

  if (!p->started) {
    // play finished, go to ready state
    q->cur_state = READY_STATE;
    /* TODO: RTP struct to be freed */
  } else if (p->transport.rtcp_sock) {
    FD_SET(Sock_fd(p->transport.rtcp_sock), &rset);
    max_fd =
      MAX(max_fd, Sock_fd(p->transport.rtcp_sock));
  }
}

/**
 * Add to the read set the current rtsp session fd.
 * The rtsp/tcp interleaving requires additional care.
 */
static void established_each_fd(gpointer data, gpointer user_data)
{
  RTSP_buffer *rtsp = (RTSP_buffer*)data;

  // FD used for RTSP connection
  FD_SET(Sock_fd(rtsp->sock), &rset);
  max_fd = MAX(max_fd, Sock_fd(rtsp->sock));
  if (g_async_queue_length(rtsp->out_queue) > 0) {
    FD_SET(Sock_fd(rtsp->sock), &wset);
  }
  // Local FDS for interleaved trasmission
  g_slist_foreach(rtsp->interleaved, interleaved_set_fds, NULL);

  // RTCP input
  if ( rtsp->session )
    g_slist_foreach(rtsp->session->rtp_sessions, rtp_session_set_fd, rtsp->session);
}

static void interleaved_close_fds(gpointer element, gpointer user_data)
{
  RTSP_interleaved *intlvd = (RTSP_interleaved *)element;

  Sock_close(intlvd->rtp_local);
  Sock_close(intlvd->rtcp_local);
  g_free(intlvd);
}

/**
 * Handle established connection and clean up in case of unexpected
 * disconnection
 */

static void established_each_connection(gpointer data, gpointer user_data)
{
  RTSP_buffer *p = (RTSP_buffer*)data;
  feng *srv = p->srv;
  GString *outbuf = NULL;
  
  int res;

  if ((res = rtsp_server(p)) == ERR_NOERROR)
    return;
  
  if (res != ERR_CONNECTION_CLOSE && res != ERR_GENERIC)
    return;

  // The connection is closed
  if (res == ERR_CONNECTION_CLOSE)
    fnc_log(FNC_LOG_INFO,
	    "RTSP connection closed by client.");
  else
    fnc_log(FNC_LOG_INFO,
	    "RTSP connection closed by server.");

  if (p->session != NULL) {
#if 0 // Do not use it, is just for testing...
    if (p->session->resource->info->multicast[0]) {
      fnc_log(FNC_LOG_INFO,
	      "RTSP connection closed by client during"
	      " a multicast session, ignoring...");
      continue;
    }
#endif

    // Release all RTP sessions
    g_slist_foreach(p->session->rtp_sessions, schedule_remove, NULL);
    g_slist_free(p->session->rtp_sessions);

    // Close connection                     
    //close(p->session->fd);
    // Release the mediathread resource
    mt_resource_close(p->session->resource);
    // Release the RTSP session
    g_free(p->session);
    p->session = NULL;
    fnc_log(FNC_LOG_WARN,
	    "WARNING! RTSP connection truncated before ending operations.\n");
  }

  // close local fds
  g_slist_foreach(p->interleaved, interleaved_close_fds, NULL);
  g_slist_free(p->interleaved);

  // Remove the output queue
  g_async_queue_lock(p->out_queue);
  while( (outbuf = g_async_queue_try_pop_unlocked(p->out_queue)) )
    g_string_free(outbuf, TRUE);
  g_async_queue_unlock(p->out_queue);
  g_async_queue_unref(p->out_queue);

  // wait for 
  Sock_close(p->sock);
  --srv->conn_count;
  srv->num_conn--;
  // Release the RTSP_buffer
  clients = g_slist_remove(clients, p);
  g_free(p);
  // Release the scheduler if necessary
  if (p == NULL && srv->conn_count < 0) {
    fnc_log(FNC_LOG_DEBUG,
	    "Thread stopped\n");
    srv->stop_schedule = 1;
  }
}

// XXX FIXME FIXME I should return something meaningful!

static void add_client(feng *srv, Sock *client_sock)
{
    RTSP_buffer *new = g_new0(RTSP_buffer, 1);

    new->srv = srv;
    new->sock = client_sock;
    new->out_queue = g_async_queue_new();

    new->session = g_new0(RTSP_session, 1);
    new->session->session_id = -1;
    new->session->srv = srv;

    clients = g_slist_prepend(clients, new);

    fnc_log(FNC_LOG_INFO,
        "Incoming RTSP connection accepted on socket: %d\n",
        Sock_fd(client_sock));
}

/**
 * Add the socket fd to the rset
 */

static void add_sock_fd(gpointer data, gpointer user_data)
{
    Sock *sock = data;

    FD_SET(Sock_fd(sock), &rset);
    max_fd = MAX(Sock_fd(sock), max_fd);
}


/**
 * @brief Search function for connections in the clients list
 * @param value Current entry
 * @param compare Socket to find in the list
 */
static gboolean connections_compare_socket(gconstpointer value, gconstpointer compare)
{
  RTSP_buffer *p = (RTSP_buffer*)value;

  return !!Sock_compare((Sock *)compare, p->sock);
}

/**
 * Accepts the new connection if possible.
 */

static void incoming_connection(gpointer data, gpointer user_data)
{
    Sock *sock = data;
    feng *srv = user_data;
    Sock *client_sock = NULL;
    GSList *p = NULL;

    if (FD_ISSET(Sock_fd(sock), &rset)) {
        client_sock = Sock_accept(sock, NULL);
    }

    // Handle a new connection
    if (!client_sock)
      return;

    p = g_slist_find_custom(clients, client_sock,
                            connections_compare_socket);

    if ( p == NULL ) {
      if (srv->conn_count < ONE_FORK_MAX_CONNECTION) {
	++srv->conn_count;
	// ADD A CLIENT
	add_client(srv, client_sock);
      } else {
	Sock_close(client_sock);
      }
      srv->num_conn++;
      fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->num_conn);

      return;
    }

    fnc_log(FNC_LOG_INFO, "Connection found: %d\n",
	    Sock_fd(client_sock));
}

/**
 * @brief Initialise data structure needed for eventloop
 */
void eventloop_init() {
  listen_socks = g_ptr_array_new();
}

/**
 * Main loop waiting for clients
 * @param srv server instance variable.
 */

void eventloop(feng *srv)
{
    //Init of scheduler
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    if (srv->conn_count != -1) {
      g_ptr_array_foreach(listen_socks, add_sock_fd, NULL);
    }

    /* Add all sockets of all sessions to rset and wset */
    g_slist_foreach(clients, established_each_fd, NULL);

    /* Wait for connections */
    if (select(max_fd + 1, &rset, &wset, NULL, NULL) < 0) {
        fnc_log(FNC_LOG_ERR, "select error in eventloop(). %s\n",
                strerror(errno));
        /* Maybe we have to force exit here*/
        return;
    }
    /* transfer data for any RTSP sessions */
    g_slist_foreach(clients, established_each_connection, NULL);

    /* handle new connections */
    if (srv->conn_count != -1) {
      g_ptr_array_foreach(listen_socks, incoming_connection, srv);
    }
}

static void free_sock(gpointer data, gpointer user_data)
{
    Sock_close(data);
}

/**
 * @brief Cleanup data structure needed by the eventloop
 */
void eventloop_cleanup() {
  g_ptr_array_foreach(listen_socks, free_sock, NULL);
  g_ptr_array_free(listen_socks, FALSE); // XXX Socks aren't g_mallocated yet
}
