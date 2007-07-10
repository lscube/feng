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

#ifndef _SERV_PREFSH
#define _SERV_PREFSH

#include <config.h>

    /* Please note (1): PROCESS_COUNT must be >=1
     */
    /* Please note (2):
       MAX_CONNECTION must be an integral multiple of MAX_PROCESS
     */
#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    100    /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

typedef enum _pref_id {
    // Preferences
    PREFS_ROOT,
    PREFS_TCP_PORT,
    PREFS_SCTP_PORT,
    PREFS_FIRST_UDP_PORT,
    PREFS_MAX_SESSION,
    PREFS_LOG,
    PREFS_HOSTNAME, 
    PREFS_USER,
    PREFS_GROUP,
    PREFS_LAST, /// Insert other preferences before this one
    PREFS_ALL  /// For prefs_use_default()
} pref_id;

    // Sync with the first preference in enum _pref_id
#define PREFS_FIRST PREFS_ROOT

typedef enum _pref_type {
    INTEGER,
    STRING
} pref_type;

typedef struct _pref_record {
    pref_id id;    /// identifies the preference
    pref_type type;    /// for selecting the right way to allocate
                        /// and accessing data
    char tag[256];    /// for storing the tag to be used when parsing
                        /// the config file
    void *data;    /// store the pointer to data
} pref_record;

    // IMPORTANT: keep the same order of enum _pref_id
#define CREATE_PREFS_DATA static pref_record prefs[PREFS_LAST] = { \
        { PREFS_ROOT, STRING, "root", NULL }, \
        { PREFS_TCP_PORT, INTEGER, "tcp_port", NULL }, \
        { PREFS_SCTP_PORT, INTEGER, "sctp_port", NULL }, \
        { PREFS_FIRST_UDP_PORT, INTEGER, "first_udp_port", NULL }, \
        { PREFS_MAX_SESSION, INTEGER, "max_session", NULL }, \
        { PREFS_LOG, STRING, "log_file", NULL }, \
        { PREFS_HOSTNAME, STRING, "###", NULL }, \
        { PREFS_USER, STRING, "user", NULL }, \
        { PREFS_GROUP, STRING, "group", NULL } \
    };

#define SET_STRING_DATA(PREF_ID, PREF_DATA) do { \
        if (prefs[(PREF_ID)].data) free(prefs[(PREF_ID)].data); \
        prefs[(PREF_ID)].data = strdup(PREF_DATA); \
    } while (0)

#define SET_INTEGER_DATA(PREF_ID, PREF_DATA) do { \
        if (prefs[(PREF_ID)].data) free(prefs[(PREF_ID)].data); \
        prefs[(PREF_ID)].data = malloc(sizeof(int)); \
        *((int *) prefs[(PREF_ID)].data) = (PREF_DATA); \
    } while (0)

// Functions
/** read the config file and fill the values in the global configuration
 * @parameter fileconf takes a string with the full path or NULL to use defaults
 */
void prefs_init(char *fileconf);
/** returns a pointer to the pref data, NULL if non existent.
 */
void *get_pref(pref_id id);
/** initializes the default prefs for the specific id
 */
void prefs_use_default(pref_id id);

// Macros (for compatibility with old prefs.h
/// XXX deprecate them 
#define prefs_get_serv_root() ((char *) get_pref(PREFS_ROOT))
#define prefs_get_port() (*((int *) get_pref(PREFS_TCP_PORT)))
#define prefs_get_sctp_port() (*((int *) get_pref(PREFS_SCTP_PORT)))
#define prefs_get_max_session() (*((int *) get_pref(PREFS_MAX_SESSION)))
#define prefs_get_log() ((char *) get_pref(PREFS_LOG))
#define prefs_get_hostname() ((char *) get_pref(PREFS_HOSTNAME))

#endif
