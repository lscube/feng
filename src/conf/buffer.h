/**
 * Copyright (c) 2004, Jan Kneschke, incremental
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the 'incremental' nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FN_BUFFER_H
#define FN_BUFFER_H

#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "settings.h"

typedef struct {
    char *ptr;

    size_t used;
    size_t size;
} conf_buffer;

typedef struct {
    conf_buffer **ptr;

    size_t used;
    size_t size;
} conf_buffer_array;

typedef struct {
    char *ptr;

    size_t offset; /* input-pointer */

    size_t used;   /* output-pointer */
    size_t size;
} conf_read_buffer;

conf_buffer* buffer_init(void);
conf_buffer* buffer_init_buffer(conf_buffer *b);
conf_buffer* buffer_init_string(const char *str);
void buffer_free(conf_buffer *b);
void buffer_reset(conf_buffer *b);

int buffer_prepare_copy(conf_buffer *b, size_t size);
int buffer_prepare_append(conf_buffer *b, size_t size);

int buffer_copy_string(conf_buffer *b, const char *s);
int buffer_copy_string_len(conf_buffer *b, const char *s, size_t s_len);
int buffer_copy_string_buffer(conf_buffer *b, const conf_buffer *src);

int buffer_copy_long(conf_buffer *b, long val);

int buffer_append_string(conf_buffer *b, const char *s);
int buffer_append_string_len(conf_buffer *b, const char *s, size_t s_len);
int buffer_append_string_buffer(conf_buffer *b, const conf_buffer *src);
int buffer_append_string_lfill(conf_buffer *b, const char *s, size_t maxlen);

int buffer_append_long(conf_buffer *b, long val);

int buffer_is_empty(conf_buffer *b);
int buffer_is_equal_string(conf_buffer *a, const char *s, size_t b_len);
int buffer_caseless_compare(const char *a, size_t a_len, const char *b, size_t b_len);

typedef enum {
    ENCODING_UNSET,
    ENCODING_REL_URI, /* for coding a rel-uri (/with space/and%percent) nicely as part of a href */
    ENCODING_REL_URI_PART, /* same as ENC_REL_URL plus coding / too as %2F */
    ENCODING_HTML,         /* & becomes &amp; and so on */
    ENCODING_MINIMAL_XML,  /* minimal encoding for xml */
    ENCODING_HEX,          /* encode string as hex */
    ENCODING_HTTP_HEADER   /* encode \n with \t\n */
} buffer_encoding_t;

#define BUFFER_APPEND_STRING_CONST(x, y) \
    buffer_append_string_len(x, y, sizeof(y) - 1)

#define BUFFER_COPY_STRING_CONST(x, y) \
    buffer_copy_string_len(x, y, sizeof(y) - 1)

#define BUFFER_APPEND_SLASH(x) \
    if (x->used > 1 && x->ptr[x->used - 2] != '/') { BUFFER_APPEND_STRING_CONST(x, "/"); }

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_BUF_LEN(x) x->ptr, x->used ? x->used - 1 : 0


#define SEGFAULT() do { fprintf(stderr, "%s.%d: aborted\n", __FILE__, __LINE__); abort(); } while(0)
#define UNUSED(x) ( (void)(x) )

#endif // FN_BUFFER_H
