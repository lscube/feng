/** ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla  Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and  limitations under the
 * License.
 *
 * The Initial Developer: Nicola Christian Barbieri 
 * Contributor(s): 
 */

#ifndef CPD_H
#define CPD_H

#define MAX_CHARS 1024
#define OK 0
#define ERROR -1
#define WARNING 1

#include "network/rtp.h"
#include "mediathread/mediathread.h"

typedef struct {
    double Timestamp;
    char *Content;
    size_t Size;
    uint8_t Sent;
} MDPacket;

typedef struct {
    Sock* Socket;
    char* Filename;
    GList* Packets;
} Metadata;

int cpd_open(Metadata *md);
void cpd_find_request(feng *srv, Resource *res, char *filename);
void cpd_send(RTP_session *session, double now);
void cpd_free_client(void *arg);
void* cpd_server(void *args);
void cpd_close(RTP_session *session);

#endif

