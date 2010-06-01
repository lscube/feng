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
#include <string.h>
#include "uri.h"

%%{
    machine uri_parser;
	include common "common.rl";

    action collapse_path {
        if ( ! g_queue_is_empty(stack) ) {
            GString *uri_path = g_string_new("");
            g_queue_foreach(stack, append_to_string, uri_path);
            uri->path = g_string_free(uri_path, FALSE);
        }
    }

	reg_name = ( unreserved | pct_encoded | sub_delims )*;
	host = IP_literal | IPv4address | reg_name;
	authority = ( userinfo >mark
                     %{uri->userinfo = g_strndup(mark, fpc-mark);}
                     "@" )?
                    host >mark
                     %{uri->host = g_strndup(mark, fpc-mark);}
                    ( ":" digit* >mark
                     %{uri->port = g_strndup(mark, fpc-mark);})?;
	pchar = unreserved | pct_encoded | sub_delims | ":" | "@";
	segment = ( pchar+ - ".." - "." ) >mark
                        %{mark = g_strndup(mark, fpc-mark);
                          g_queue_push_tail(stack, mark);}|
                  ".." %{g_queue_pop_tail(stack);}|
                    zlen | ".";
	path_abempty = ( "/" segment )*;
	segment_nz = ( pchar+ - ".." ) | "..";
	path_absolute = "/" ( segment_nz ( "/" segment )* )?;
	path_rootless = segment_nz ( "/" segment )*;
	path_empty = empty;
	hier_part = ( "//" authority path_abempty ) | path_absolute | path_rootless | path_empty;
	query = ( pchar | "/" | "?" )*;
	fragment = ( pchar | "/" | "?" )*;
	URI = scheme >mark
               %{uri->scheme = g_strndup(mark, fpc-mark);}
              ":" hier_part ( "?" query >mark
               %{uri->query = g_strndup(mark, fpc-mark);}
                )? ( "#" fragment >mark
               %{uri->fragment = g_strndup(mark, fpc-mark);} )?
               %(collapse_path);
	segment_nz_nc = ( unreserved | pct_encoded | sub_delims | "@" )+;
	path_noscheme = segment_nz_nc ( "/" segment )*;
	relative_part = ( "//" authority path_abempty ) | path_absolute | path_noscheme | path_empty;
	relative_ref = relative_part ( "?" query )? ( "#" fragment )?;
	URI_reference = URI | relative_ref;
	absolute_URI = scheme ":" hier_part ( "?" query )?;
	path = path_abempty | path_absolute | path_noscheme | path_rootless | path_empty;
	gen_delims = ":" | "/" | "?" | "#" | "[" | "]" | "@";
	reserved = gen_delims | sub_delims;

    # instantiate machine rules
    main:= URI 0;
}%%

static void append_to_string(gpointer data, gpointer user_data)
{
    GString *path = (GString*)user_data;
    char *segment = (char *)data;
    g_string_append_printf(path, "/%s", segment);
}

/**
 * @brief Parse and split an URI string into an URI structure
 *
 * @param uri_string The URI string to parse and split
 *
 * @return A pointer to a newly-allocated URI structure.
 *
 * This function parses an URI, as defined by RFC1738, and splits it
 * into its components in the given structure.
 */
URI *uri_parse(const char *uri_string)
{
    const char *p = uri_string, *pe = p + strlen(p) + 1, *mark;
    GQueue *stack = g_queue_new();
    URI *uri = g_slice_new0(URI);
    int cs;

    %% write data;
    %% write init;
    %% write exec;

    g_queue_free(stack);

    if (cs < uri_parser_first_final) {
        uri_free(uri);
        return NULL;
    }

    return uri;
}
