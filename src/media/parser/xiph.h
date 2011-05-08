/*
 * This file is part of Feng
 *
 * Copyright (C) 2011 by LScube team <team@lscube.org>
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
 */

#ifndef FN_PARSER_XIPH_H
#define FN_PARSER_XIPH_H

typedef struct {
    uint8_t        *conf;      ///< current configuration
    size_t          conf_len;
    uint8_t        *packet;    ///< holds the incomplete packet
    size_t          len;       ///< incomplete packet length
    unsigned int    ident;     ///< identification string
} xiph_priv;

int xiph_parse(Track *tr, uint8_t *data, size_t len);
void xiph_uninit(Track *tr);

#endif
