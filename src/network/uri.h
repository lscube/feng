/* This file is part of feng
 *
 * Copyright (C) 2008-2010 by LScube team <team@streaming.polito.it>
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

/**
 * @file
 * @brief URI utility functions
 *
 * This file includes the definition of the Eris URI structure and the
 * functions to be used with that.
 */

#ifndef FN_URI_H
#define FN_URI_H

#include <glib.h>

/**
 * @defgroup uri URI handling functions
 *
 * This group includes all the public definitions that relates to URI
 * handling (structures, parsers, composers, validators, ...).
 *
 * @{
 */

/**
 * @brief URI structure used inside of Eris
 *
 * This structure encapsulate the basic fragments composing an URI
 * strings.
 */
typedef struct URI {
    /** URI scheme, otherwise called protocol */
    char *scheme;
    /** User information (username and password) */
    char *userinfo;
    /** Hostname */
    char *host;
    /** Port on the hostname */
    char *port;
    /** Path of the URI */
    char *path;
    /** Query part of the URI (after ?) */
    char *query;
    /** Fragment part of the URI (after #) */
    char *fragment;
} URI;

URI *uri_parse(const char *uri_string);
URI *uri_validate(const char *urlstr);

void uri_free(URI *uri);

/**
 * @}
 */

#endif
