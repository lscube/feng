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

#include "config.h"

#include "utils.h"

/**
 * @brief Creates a new headers table (GHashTable).
 *
 * @return A new GHashTable object ready to contain headers.
 *
 * This function is a simple thin wrapper around
 * g_hash_table_new_full() that already provides the right parameters
 * to the function.
 *
 * @internal This function is to be used only by liberis functions.
 */
GHashTable *_eris_hdr_table_new()
{
    return g_hash_table_new_full(g_str_hash, g_str_equal,
                                 g_free, g_free);
}
