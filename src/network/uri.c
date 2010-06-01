/* -*-c-*- */
/* This file is part of liberis
 *
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 *
 * liberis is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * liberis is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with liberis.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "uri.h"

void uri_free(URI *uri)
{
    g_free(uri->scheme);
    g_free(uri->userinfo);
    g_free(uri->host);
    g_free(uri->port);
    g_free(uri->path);
    g_free(uri->query);
    g_free(uri->fragment);

    g_slice_free(URI, uri);
}
