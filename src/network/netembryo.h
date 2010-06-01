/*
 * This file is part of feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * NetEmbryo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NetEmbryo; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FN_NETEMBRYO_H__
#define FN_NETEMBRYO_H__

#include <netinet/in.h>
#include <sys/socket.h>

#define NETEMBRYO_MAX_SCTP_STREAMS 15

char *neb_sa_get_host(const struct sockaddr *sa);
in_port_t neb_sa_get_port(struct sockaddr *sa);
void neb_sa_set_port(struct sockaddr *sa, in_port_t port);

#endif
