/* *
 *  This file is part of Feng
 *
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 *
 * Feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#ifndef CPD_H
#define CPD_H

#define MAX_CHARS 1024
#define OK 0
#define ERROR -1
#define WARNING 1

#include "network/rtp.h"

typedef struct {
    double Timestamp;
    char *Content;
    size_t Size;
    uint8_t Sent;
} CPDPacket;

typedef struct {
    Sock* Socket;
    char* Filename;
    GList* Packets;
} CPDMetadata;

int cpd_open(CPDMetadata *md);
void cpd_find_request(feng *srv, Resource *res, char *filename);
void cpd_send(RTP_session *session, double now);
void cpd_free_client(void *arg);
void* cpd_server(void *args);
void cpd_close(RTP_session *session);

#endif

