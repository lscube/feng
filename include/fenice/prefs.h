/* * 
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
 * */

#ifndef FN_SERV_PREFS_H
#define FN_SERV_PREFS_H

#include <config.h>
//#include <fenice/server.h>
    /* Please note (1): PROCESS_COUNT must be >=1
     */
    /* Please note (2):
       MAX_CONNECTION must be an integral multiple of MAX_PROCESS
     */
#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    100    /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

typedef enum {
    // Preferences
    PREFS_ROOT = 0,
    PREFS_TCP_PORT,
    PREFS_SCTP_PORT,
    PREFS_SSL_PORT,
    PREFS_FIRST_UDP_PORT,
    PREFS_MAX_SESSION,
    PREFS_BUFFERED_FRAMES,
    PREFS_LOG,
    PREFS_HOSTNAME,
    PREFS_USER,
    PREFS_GROUP,
    PREFS_ALL  /// For prefs_use_default()
} pref_id;

// Sync with the first preference in enum _pref_id
#define PREFS_FIRST PREFS_ROOT
#define PREFS_LAST PREFS_ALL

typedef enum {
    INTEGER,
    STRING
} pref_type;

typedef struct {
    pref_type type;    /// for selecting the right way to allocate
                        /// and accessing data
    char tag[256];    /// for storing the tag to be used when parsing
                        /// the config file
    void *data;    /// store the pointer to data
} pref_record;

#define SET_STRING_DATA(PREF_ID, PREF_DATA) do { \
        if (srv->prefs[(PREF_ID)].data) free(srv->prefs[(PREF_ID)].data); \
        srv->prefs[(PREF_ID)].data = strdup(PREF_DATA); \
    } while (0)

#define SET_INTEGER_DATA(PREF_ID, PREF_DATA) do { \
        if (srv->prefs[(PREF_ID)].data) free(srv->prefs[(PREF_ID)].data); \
        srv->prefs[(PREF_ID)].data = malloc(sizeof(int)); \
        *((int *) srv->prefs[(PREF_ID)].data) = (PREF_DATA); \
    } while (0)

// Functions

struct feng;

/** read the config file and fill the values in the global configuration
 * @parameter fileconf takes a string with the full path or NULL to use defaults
 */
void prefs_init(struct feng *srv, char *fileconf);
/** returns a pointer to the pref data, NULL if non existent.
 */
void *get_pref(struct feng *srv, pref_id id);
#define get_pref_int(X) (*((int *) get_pref(srv, X)))
#define get_pref_str(X) ((char *) get_pref(srv, X))
/** initializes the default prefs for the specific id
 */
void prefs_use_default(struct feng *srv, pref_id id);

// Macros (for compatibility with old prefs.h
/// XXX deprecate them 
#define prefs_get_serv_root() srv->config_storage[0]->document_root->ptr
#define prefs_get_port() srv->srvconf.port
#define prefs_get_sctp_port() (-1) //FIXME
#define prefs_get_max_session() srv->srvconf.max_conns
#define prefs_get_log() srv->srvconf.errorlog_file->ptr
#define prefs_get_hostname() srv->srvconf.bindhost->ptr

#endif // FN_SERV_PREFS_H
