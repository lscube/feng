/* *
 * This file is part of Feng
 *
 * Copyright (C) 2010 by LScube team <team@lscube.org>
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

#include <config.h>

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#include "cfgparser.h"

#include "feng.h"
#include "fnc_log.h"
#include "network/rtp.h"

static const char *cfg_file_name;

/**
 * @brief Simple reporting function to output to standard output
 *
 * @TODO This might be better wrapped up using a pre-logging function.
 */
void yyerror(const char *fmt, ...)
{
    va_list vl;
    char *msg;

    va_start(vl, fmt);
    msg = g_strdup_vprintf(fmt, vl);
    va_end(vl);

    fnc_log(FNC_LOG_FATAL, "%s:%d: %s",
            cfg_file_name, yylineno, msg);
}

/* We don't want to abuse memory by duplicating static strings, but
 * when doing debug builds we want clean memory tracks, so depending
 * on what kind of build we're doing, do one or the other.
 */
#ifndef NDEBUG
# define cfg_default_string(x) g_strdup(x)
#else
# define cfg_default_string(x) (x)
#endif

/**
 * @brief Callback function called after the options section has been parsed.
 *
 * @param section The parsed options section
 *
 * This function _also_ sets the @ref feng_srv structure, and is the
 * only one doing so directly, since there cannot be another, and no
 * reactive action is needed other than filling the structure.
 */
bool cfg_options_callback(cfg_options_t *section)
{
    if ( section->username == NULL )
        section->username = cfg_default_string("feng");
    if ( section->groupname == NULL )
        section->groupname = cfg_default_string("feng");

    if ( section->buffered_frames == 0 )
        section->buffered_frames = 16;

    if ( section->log_level == 0 )
        section->log_level = FNC_LOG_WARN;

    if ( section->error_log == NULL )
        section->error_log = cfg_default_string("stderr");

    memcpy(&feng_srv, section, sizeof(cfg_options_t));

    memset(section, 0, sizeof(*section));
    return true;
}

/**
 * @brief Callback function called after a socket section has been parsed.
 *
 * @param section The parsed socket section
 *
 * This function simply queues a copy of the @ref cfg_socket_t
 * structure in a list of parsed sections that will be acted upon at a
 * later time.
 */
bool cfg_socket_callback(cfg_socket_t *section)
{
    if ( section->port == NULL ) {
        yyerror("missing port in socket declaration");
        return false;
    }

    configured_sockets = g_list_append(configured_sockets,
                                       g_slice_dup(cfg_socket_t, section));

    memset(section, 0, sizeof(*section));
    return true;
}

/**
 * @brief Callback function called after a vhost section has been parsed.
 *
 * @param section The parsed vhost section
 *
 * This function simply queues a copy of the @ref cfg_vhost_t
 * structure in a list of parsed sections that will be acted upon at a
 * later time.
 */
bool cfg_vhost_callback(cfg_vhost_t *section)
{
    if ( section->document_root == NULL ) {
        yyerror("missing document-root in vhost declaration");
        return false;
    }

    /* The first vhost may not have any alias at all, but any other
       will need one, error out if that's not the case. */
    if ( configured_vhosts != NULL &&
         g_list_length(section->aliases) < 1 ) {
        yyerror("missing aliases in vhost declaration");
        return false;
    }

    if ( section->access_log == NULL )
        section->access_log = cfg_default_string("stderr");

    if ( section->max_connections == 0 )
        section->max_connections = FENG_MAX_SESSION_DEFAULT;

    configured_vhosts = g_list_append(configured_vhosts,
                                      g_slice_dup(cfg_vhost_t, section));

    memset(section, 0, sizeof(*section));
    return true;
}

/**
 * @brief Parse the configuration file
 *
 * @param file The path to the filename to read, if NULL, the default
 *             path chosen at build time will be used.
 * @param lint If true, no action will be taken, and instead the
 *             syntax of the file will be tested.
 */
void config_file_parse(const char *file, bool lint)
{
    if ( file == NULL )
        file = FENG_CONF_PATH_DEFAULT_STR;

    cfg_file_name = file;

    if ( (yyin = fopen(file, "r")) == NULL ) {
        yyerror("unable to open '%s': %s",
                file,
                /* we can use the non-reentrant function since we're
                   in pre-threaded environment */
                strerror(errno));
        exit(1);
    }

    /* nothing to cleanup if we're exiting in error already */
    if ( yyparse() != 0 )
        exit(1);

    /* if we're just checking syntax, exit right away. */
    if ( lint ) exit(0);

    fclose(yyin);
    yylex_destroy();
}
