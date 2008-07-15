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

const char *host     = "localhost";
const char *user     = "javauser";
const char *password = "javadude";
const char *table    = "chillout_cpd";

const char *port     = "30000";

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <sys/select.h>
#include <fenice/fnc_log.h>
#include <metadata/cpd.h>
// dev'essere l'ultimo include
#include <mysql/mysql.h>

void cpd_find_request(feng *srv, Resource *res, char *filename) {
    // controlla se l'utente ha richiesto la risorsa AV associata
    // restituisce la chiave dell'hash table (socket_descriptor) se trova l'utente
    // viene chiamata da mediathread

    G_LOCK (g_hash_global);

    gpointer key, value;
    GHashTableIter iter;
    GHashTable *clients = (GHashTable *) srv->metadata_clients;

    fnc_log(FNC_LOG_INFO, "[CPD] Looking for Metadata request");

    g_hash_table_iter_init (&iter, clients);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
	Metadata *md = value;
	if (!strncmp(filename, md->Filename, min(sizeof(filename), sizeof(md->Filename)))) {
	    fnc_log(FNC_LOG_INFO, "[CPD] Metadata request found");
	    res->metadata = md;
	} else {
	    res->metadata = NULL;
	    fnc_log(FNC_LOG_INFO, "[CPD] Metadata request not found");
	}
    }

    G_UNLOCK (g_hash_global);

}

int cpd_open(Metadata *md) {

    G_LOCK(g_db_global);

    MYSQL mysql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char sql_query[MAX_CHARS];
    char realpath[MAX_CHARS];

    fnc_log(FNC_LOG_INFO, "[CPD] Initializing resource: '%s'", md->Filename); 

    mysql_init(&mysql);

    if (!mysql_real_connect(&mysql, host, user, password, table, 0, NULL,CLIENT_IGNORE_SIGPIPE))
    {
	fnc_log(FNC_LOG_FATAL, "[CPD] Failed to connect to MySQL. Error: %s", mysql_error(&mysql));
        return ERROR;
    } else {
	//mysql_db_change
    }

    fnc_log(FNC_LOG_VERBOSE, "[CPD] Logged on to database sucessfully");

    // look for actual resource
    char buf[MAX_CHARS];
    strcpy(buf, FENICE_AVROOT_DIR_DEFAULT_STR);
    strncat(buf, "/", sizeof(buf)-strlen(buf));
    strncat(buf, md->Filename, sizeof(buf)-strlen(buf)); 
    int nchars = readlink(buf, realpath, MAX_CHARS);
    if (nchars < 0 || nchars == MAX_CHARS) {
	fnc_log(FNC_LOG_FATAL, "[CPD] Failed to follow symbolic link.");
        return ERROR;
    }
    realpath[nchars] = '\0';

    fnc_log(FNC_LOG_VERBOSE, "[CPD] Found actual resource: '%s'", realpath); 


    strcpy(sql_query, "SELECT * FROM tmetadatapacket JOIN tresourceinfo ON tmetadatapacket.ResourceId = tresourceinfo.Id WHERE tresourceinfo.ResourcePath = '");
    strncat(sql_query, realpath, sizeof(sql_query)-strlen(sql_query));
    strncat(sql_query, "' ORDER BY tmetadatapacket.PresentationTime ASC", sizeof(sql_query)-strlen(sql_query));

    fnc_log(FNC_LOG_VERBOSE,"[CPD] Query: '%s'", sql_query);

    if (mysql_real_query (&mysql, sql_query, strlen(sql_query)) != 0) {
	mysql_close (&mysql);
	fnc_log(FNC_LOG_FATAL, "[CPD] Failed to Query DB. Error: %s", mysql_error(&mysql));
        return ERROR;
    }

    /* store the result */
    if ( (res = mysql_store_result (&mysql)) == NULL) {
	mysql_close (&mysql);
	fnc_log(FNC_LOG_FATAL, "[CPD] Failed to Store Result from DB. Error: %s", mysql_error(&mysql));
        return ERROR;
    }

    /* Check if there is some data */
    if (mysql_num_rows(res) == 0) {
	mysql_close(&mysql);
	fnc_log(FNC_LOG_WARN, "[CPD] Warning: No Metadata!");
        return WARNING;
    }

    // create packet list
    md->Packets = g_list_alloc();
    

    // fetch rows
    while  ( (row = mysql_fetch_row (res)) != NULL) {

	//fetch lengths
	unsigned long* lengths = mysql_fetch_lengths(res);

	// create packet
	MDPacket *myPacket = g_new0(MDPacket, 1);
	
	// set timestamp
	myPacket->Timestamp = strtol(row[1], NULL, 10);
	myPacket->Timestamp /= 1000;

	//set content
	myPacket->Content = g_new0(char, lengths[2] + 1);
	memcpy (myPacket->Content, row[2], lengths[2]);

	// set size
	myPacket->Size = lengths[2];

	// set Sent to false
	myPacket->Sent = 0;

	fnc_log(FNC_LOG_VERBOSE,"[CPD] Content: ts=%lf len=%lu '%s'", myPacket->Timestamp, myPacket->Size, myPacket->Content);

	// append to list
	md->Packets = g_list_append (md->Packets, myPacket);
    }

    /* free the result */
    mysql_free_result (res);

    /* close the connection */
    mysql_close (&mysql);

    G_UNLOCK(g_db_global);

    return OK;

}

void cpd_free_element(void *arg, void *fake) {
    MDPacket *packet = arg;
    if(packet) {
	g_free (packet->Content);
	g_free (packet);
    }
}

void cpd_free_client(void *arg) {
    Metadata *md = arg;

    // chiudo il socket
    Sock_close(md->Socket);

    // free della struttura metadata
    free(md->Filename);
    g_list_foreach(md->Packets, (GFunc)cpd_free_element, NULL);
    g_list_free (md->Packets);
    g_free (md);
}

int cpd_connection_alive (RTP_session *session, Sock *socket) {

    G_LOCK (g_hash_global);
    GHashTable *clients = (GHashTable *) session->srv->metadata_clients;
    if (g_hash_table_lookup(clients, &Sock_fd(socket))) {
	G_UNLOCK (g_hash_global);
	return OK;
    } else {
	G_UNLOCK (g_hash_global);
	return ERROR;
    }

}

void cpd_send(RTP_session *session, double now) {

    //fnc_log(FNC_LOG_INFO,"[CPD] Streaming Metadata: playing time %f", now - session->start_time + session->seek_time);
    double timestamp = now - session->start_time + session->seek_time;
    Metadata *md = session->Metadata;
    GList *i;

    if (cpd_connection_alive(session, md->Socket)==ERROR) {
	fnc_log(FNC_LOG_INFO, "[CPD] Metadata Connection closed by client");
	session->Metadata = NULL;
	return;
    }

    if (md->Packets)
    for (i = g_list_first (md->Packets); i ; i = g_list_next (i)) {
	// i->data
	MDPacket *packet = (MDPacket *) i->data;
	if (packet)
	if (timestamp >= packet->Timestamp && !packet->Sent)
	{
	    Sock_write(md->Socket, packet->Content, strlen(packet->Content), NULL, 0);
	    packet->Sent = 1;
	}
    }
}

void *cpd_server(void *args) {
    Sock *cpd_srv = NULL;
    fd_set read_fds;
    int max_fd;
    GHashTable* clients;

    GHashTableIter iter;
    gpointer key, value;

    //int nbytes;

    char buffer[MAX_CHARS];
    char filename[MAX_CHARS];

    feng *main_srv = (feng *) args;

    fnc_log(FNC_LOG_INFO, "[CPD] Initializing CPD");

    // creo la hashtable
    clients = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, cpd_free_client);
    main_srv->metadata_clients = clients;

    // apro il socket
    if(!(cpd_srv = Sock_bind("localhost", "30000", NULL, TCP, NULL))) {
	fnc_log(FNC_LOG_FATAL, "[CPD] Failed to create Metadata Socket");
        abort();
    }

    if(Sock_listen(cpd_srv, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "[CPD] Sock_listen() error for TCP socket.");
        fprintf(stderr, "[CPD] Sock_listen() error for TCP socket.\n");
        abort();
    }

    fnc_log(FNC_LOG_INFO, "[CPD] Listening on port %s", port);

    while(1) {
        FD_ZERO(&read_fds);
	FD_SET(Sock_fd(cpd_srv), &read_fds);
	max_fd = Sock_fd(cpd_srv);

	G_LOCK (g_hash_global);

	g_hash_table_iter_init (&iter, clients);

        while (g_hash_table_iter_next (&iter, &key, NULL)) 
        {
	    int fd = *(int *)key;
	    FD_SET(fd, &read_fds);
	    if (fd>max_fd)
		max_fd = fd;
        }
	
	G_UNLOCK (g_hash_global);


	// TODO: ciclo for che aggiunge i socket volta per volta
	select(max_fd+1, &read_fds, NULL, NULL, NULL);
	if (FD_ISSET(Sock_fd(cpd_srv), &read_fds)) {
		Sock* new_sd = Sock_accept(cpd_srv, NULL);
		Metadata* md = g_new0(Metadata, 1); 
		md->Socket = new_sd;
		//md->ResourceId = strdup(resourceId);
		G_LOCK (g_hash_global);
		g_hash_table_insert(clients, &Sock_fd(new_sd), md);
		fnc_log(FNC_LOG_INFO, "[CPD] Incoming connection on socket : %d\n", Sock_fd(md->Socket));
		G_UNLOCK (g_hash_global);
		
	}

	G_LOCK (g_hash_global);

	g_hash_table_iter_init (&iter, clients);
        while (g_hash_table_iter_next (&iter, &key, &value)) 
        {
	    int fd = *((int *)key);
	    Metadata *md = value;
	    if (FD_ISSET(fd, &read_fds)) {
		// verifying
		int result = Sock_read(md->Socket, buffer, sizeof(buffer), NULL, 0);
		if (result<=0) {
		    // client left
		    fnc_log(FNC_LOG_INFO, "[CPD] Closing connection on socket : %d\n", Sock_fd(md->Socket));
		    g_hash_table_remove (clients, key);
		    // g_free 
		} else {

		    // receiving data
		    fnc_log(FNC_LOG_INFO, "[CPD] Metadata Request received"); 

		    if (strncmp(buffer, "REQUEST ", min(sizeof(buffer), 8))) {
			// Command not recognized
			fnc_log(FNC_LOG_INFO, "[CPD] Invalid request");
			fnc_log(FNC_LOG_INFO, "[CPD] Closing connection on socket : %d\n", Sock_fd(md->Socket));
			strcpy(buffer, "INVALID REQUEST\n");
			Sock_write(md->Socket, buffer, strlen(buffer), NULL, 0);
			g_hash_table_remove (clients, key);
		    } else {
			sscanf(buffer+8, "%1015s", filename);
			md->Filename = strdup(filename);
			// TODO: find in DB
			switch (cpd_open(md)) {
			    case OK:
			        // TODO: register RTP Session handler 
			        strcpy(buffer, "SUCCESS\n");
			        Sock_write(md->Socket, buffer, strlen(buffer), NULL, 0);
				fnc_log(FNC_LOG_INFO, "[CPD] Request accepted");
				break;
			    case ERROR:
			    case WARNING:
			    default:
				fnc_log(FNC_LOG_INFO, "[CPD] Request discarded");
				fnc_log(FNC_LOG_INFO, "[CPD] Closing connection on socket : %d\n", Sock_fd(md->Socket));
			        strcpy(buffer, "NOT FOUND\n");
			        Sock_write(md->Socket, buffer, strlen(buffer), NULL, 0);
			        g_hash_table_remove (clients, key);
				break;
			}
			
		    }
		}
	    }
        }
	G_UNLOCK (g_hash_global);


    }
}

void cpd_close(RTP_session *session) {
    G_LOCK (g_hash_global);
    GHashTable *clients = (GHashTable *) session->srv->metadata_clients;
    Metadata *md = (Metadata *) session->Metadata;
    fnc_log(FNC_LOG_INFO, "[CPD] Closing connection on socket : %d\n", Sock_fd(md->Socket));
    g_hash_table_remove (clients, &Sock_fd(md->Socket));
    G_UNLOCK (g_hash_global);
}
