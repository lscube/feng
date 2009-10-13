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

#include "config.h"

#include "liberis/headers.h"
#include "liberis/utils.h"

%%{
  machine headers_parser;
  include common "common.rl";

  action hdr_start {
    hdr = p;
  }

  action hdr_end {
    hdr_size = p-hdr;
  }

  action hdr_val_start {
    hdr_val = p;
  }

  action hdr_val_end {
    hdr_val_size = p-hdr_val;
  }

  Header = unreserved+ > hdr_start % hdr_end
      . ':' . SP . print+ > hdr_val_start % hdr_val_end . CRLF;

  action save_header {
      g_hash_table_insert(*table,
                          g_strndup(hdr, hdr_size),
                          g_strndup(hdr_val, hdr_val_size));
  }

  action exit_parser {
      return p-hdrs_string;
  }

  main := (Header @ save_header)+ . CRLF % exit_parser . /.*/;

}%%

/**
 * @brief Parse a string containing RTSP headers in a GHashTable
 * @param hdrs_string The string containing the series of headers
 * @param len Length of the parameters string, included final newline.
 * @param table Pointer where to write the actual GHashTable instance
 *              pointer containing the parsed headers.
 *
 * @return The number of characters parsed from the input
 *
 * This function can be used to parse RTSP request, response and
 * entity headers into an hash table that can then be used to access
 * them.
 *
 * The reason why this function takes a pointer and a size is that it
 * can be used directly from a different Ragel-based parser, like the
 * request or response parser, where the string is not going to be
 * NULL-terminated.
 *
 * Whenever the returned size is zero, the parser encountered a
 * problem and couldn't complete the parsing, the @p table parameter
 * is undefined.
 */
size_t eris_parse_headers(const char *hdrs_string, size_t len, GHashTable **table)
{
  const char *p = hdrs_string, *pe = p + len, *eof = pe;

  int cs, line;

  const char *hdr = NULL, *hdr_val = NULL;
  size_t hdr_size = 0, hdr_val_size = 0;

  /* Create the new hash table */
  *table = _eris_hdr_table_new();

  {
    %% write data noerror;
    %% write init;
    %% write exec;

    if ( cs < headers_parser_first_final ) {
      g_hash_table_destroy(*table);
      return 0;
    }

    cs = headers_parser_en_main;
  }

  return p-hdrs_string;
}
