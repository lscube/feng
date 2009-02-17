/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#ifndef FN_SDP_GRAMMAR_H
#define FN_SDP_GRAMMAR_H

#include <glib.h>

typedef enum {empty_field, fmtp, rtpmap} sdp_field_types;

typedef GList *sdp_field_list;

#define SDP_FIELD(x) ((sdp_field *)x->data)

typedef struct {
	sdp_field_types type;
	char *field;
} sdp_field;

#endif // FN_SDP_GRAMMAR_H
