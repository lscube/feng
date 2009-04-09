/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#ifndef FN_SDP_H
#define FN_SDP_H

#include <glib.h>
#include <netembryo/url.h>

/**
 * @brief Separator for the track ID on presentation URLs
 *
 * This string separates the resource URL from the track name in a
 * presentation URL, for SETUP requests for instance.
 *
 * It is represented as a preprocessor macro to be easily used both to
 * glue together the names and to use it to find the track name.
 */
#define SDP_TRACK_SEPARATOR "TrackID="

/**
 * @brief Convenience macro for the track separator in URLs
 *
 * Since outside of the SDP code we need to check for a '/' prefix,
 * just write it here once.
 */
#define SDP_TRACK_URI_SEPARATOR "/" SDP_TRACK_SEPARATOR

struct feng;

GString *sdp_session_descr(struct feng *srv, const Url *url);

#endif // FN_SDP_H
