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

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "buffer.h"


/**
 * init the buffer
 *
 */

conf_buffer* buffer_init(void) {
    conf_buffer *b;

    b = malloc(sizeof(*b));
    assert(b);

    b->ptr = NULL;
    b->size = 0;
    b->used = 0;

    return b;
}

conf_buffer *buffer_init_buffer(conf_buffer *src) {
    conf_buffer *b = buffer_init();
    buffer_copy_string_buffer(b, src);
    return b;
}

/**
 * free the buffer
 *
 */

void buffer_free(conf_buffer *b) {
    if (!b) return;

    free(b->ptr);
    free(b);
}

void buffer_reset(conf_buffer *b) {
    if (!b) return;

    /* limit don't reuse buffer larger than ... bytes */
    if (b->size > BUFFER_MAX_REUSE_SIZE) {
        free(b->ptr);
        b->ptr = NULL;
        b->size = 0;
    }

    b->used = 0;
}


/**
 *
 * allocate (if neccessary) enough space for 'size' bytes and
 * set the 'used' counter to 0
 *
 */

#define BUFFER_PIECE_SIZE 64

int buffer_prepare_copy(conf_buffer *b, size_t size) {
    if (!b) return -1;

    if ((0 == b->size) ||
        (size > b->size)) {
        if (b->size) free(b->ptr);

        b->size = size;

        /* always allocate a multiply of BUFFER_PIECE_SIZE */
        b->size += BUFFER_PIECE_SIZE - (b->size % BUFFER_PIECE_SIZE);

        b->ptr = malloc(b->size);
        assert(b->ptr);
    }
    b->used = 0;
    return 0;
}

/**
 *
 * increase the internal buffer (if neccessary) to append another 'size' byte
 * ->used isn't changed
 *
 */

int buffer_prepare_append(conf_buffer *b, size_t size) {
    if (!b) return -1;

    if (0 == b->size) {
        b->size = size;

        /* always allocate a multiply of BUFFER_PIECE_SIZE */
        b->size += BUFFER_PIECE_SIZE - (b->size % BUFFER_PIECE_SIZE);

        b->ptr = malloc(b->size);
        b->used = 0;
        assert(b->ptr);
    } else if (b->used + size > b->size) {
        b->size += size;

        /* always allocate a multiply of BUFFER_PIECE_SIZE */
        b->size += BUFFER_PIECE_SIZE - (b->size % BUFFER_PIECE_SIZE);

        b->ptr = realloc(b->ptr, b->size);
        assert(b->ptr);
    }
    return 0;
}

int buffer_copy_string(conf_buffer *b, const char *s) {
    size_t s_len;

    if (!s || !b) return -1;

    s_len = strlen(s) + 1;
    buffer_prepare_copy(b, s_len);

    memcpy(b->ptr, s, s_len);
    b->used = s_len;

    return 0;
}

int buffer_copy_string_len(conf_buffer *b, const char *s, size_t s_len) {
    if (!s || !b) return -1;
#if 0
    /* removed optimization as we have to keep the empty string
     * in some cases for the config handling
     *
     * url.access-deny = ( "" )
     */
    if (s_len == 0) return 0;
#endif
    buffer_prepare_copy(b, s_len + 1);

    memcpy(b->ptr, s, s_len);
    b->ptr[s_len] = '\0';
    b->used = s_len + 1;

    return 0;
}

int buffer_copy_string_buffer(conf_buffer *b, const conf_buffer *src) {
    if (!src) return -1;

    if (src->used == 0) {
        b->used = 0;
        return 0;
    }
    return buffer_copy_string_len(b, src->ptr, src->used - 1);
}

int buffer_append_string(conf_buffer *b, const char *s) {
    size_t s_len;

    if (!s || !b) return -1;

    s_len = strlen(s);
    buffer_prepare_append(b, s_len + 1);
    if (b->used == 0)
        b->used++;

    memcpy(b->ptr + b->used - 1, s, s_len + 1);
    b->used += s_len;

    return 0;
}

/**
 * append a string to the end of the buffer
 *
 * the resulting buffer is terminated with a '\0'
 * s is treated as a un-terminated string (a '\0' is handled a normal character)
 *
 * @param b a buffer
 * @param s the string
 * @param s_len size of the string (without the terminating '\0')
 */

int buffer_append_string_len(conf_buffer *b, const char *s, size_t s_len) {
    if (!s || !b) return -1;
    if (s_len == 0) return 0;

    buffer_prepare_append(b, s_len + 1);
    if (b->used == 0)
        b->used++;

    memcpy(b->ptr + b->used - 1, s, s_len);
    b->used += s_len;
    b->ptr[b->used - 1] = '\0';

    return 0;
}

int buffer_append_string_buffer(conf_buffer *b, const conf_buffer *src) {
    if (!src) return -1;
    if (src->used == 0) return 0;

    return buffer_append_string_len(b, src->ptr, src->used - 1);
}

static int LI_ltostr(char *buf, long val) {
    char swap;
    char *end;
    int len = 1;

    if (val < 0) {
        len++;
        *(buf++) = '-';
        val = -val;
    }

    end = buf;
    while (val > 9) {
        *(end++) = '0' + (val % 10);
        val = val / 10;
    }
    *(end) = '0' + val;
    *(end + 1) = '\0';
    len += end - buf;

    while (buf < end) {
        swap = *end;
        *end = *buf;
        *buf = swap;

        buf++;
        end--;
    }

    return len;
}

int buffer_append_long(conf_buffer *b, long val) {
    if (!b) return -1;

    buffer_prepare_append(b, 32);
    if (b->used == 0)
        b->used++;

    b->used += LI_ltostr(b->ptr + (b->used - 1), val);
    return 0;
}

int buffer_copy_long(conf_buffer *b, long val) {
    if (!b) return -1;

    b->used = 0;
    return buffer_append_long(b, val);
}

conf_buffer *buffer_init_string(const char *str) {
    conf_buffer *b = buffer_init();

    buffer_copy_string(b, str);

    return b;
}

int buffer_is_empty(conf_buffer *b) {
    if (!b) return 1;
    return (b->used == 0);
}

/**
 * check if two buffer contain the same data
 *
 * HISTORY: this function was pretty much optimized, but didn't handled
 * alignment properly.
 */

static int buffer_is_equal(conf_buffer *a, conf_buffer *b) {
    if (a->used != b->used) return 0;
    if (a->used == 0) return 1;

    return (0 == strcmp(a->ptr, b->ptr));
}

int buffer_is_equal_string(conf_buffer *a, const char *s, size_t b_len) {
    conf_buffer b;

    b.ptr = (char *)s;
    b.used = b_len + 1;

    return buffer_is_equal(a, &b);
}

/* simple-assumption:
 *
 * most parts are equal and doing a case conversion needs time
 *
 */
int buffer_caseless_compare(const char *a, size_t a_len, const char *b, size_t b_len) {
    size_t ndx = 0, max_ndx;
    size_t *al, *bl;
    size_t mask = sizeof(*al) - 1;

    al = (size_t *)a;
    bl = (size_t *)b;

    /* is the alignment correct ? */
    if ( ((size_t)al & mask) == 0 &&
         ((size_t)bl & mask) == 0 ) {

        max_ndx = ((a_len < b_len) ? a_len : b_len) & ~mask;

        for (; ndx < max_ndx; ndx += sizeof(*al)) {
            if (*al != *bl) break;
            al++; bl++;

        }

    }

    a = (char *)al;
    b = (char *)bl;

    max_ndx = ((a_len < b_len) ? a_len : b_len);

    for (; ndx < max_ndx; ndx++) {
        char a1 = *a++, b1 = *b++;

        if (a1 != b1) {
            if ((a1 >= 'A' && a1 <= 'Z') && (b1 >= 'a' && b1 <= 'z'))
                a1 |= 32;
            else if ((a1 >= 'a' && a1 <= 'z') && (b1 >= 'A' && b1 <= 'Z'))
                b1 |= 32;
            if ((a1 - b1) != 0) return (a1 - b1);

        }
    }

    /* all chars are the same, and the length match too
     *
     * they are the same */
    if (a_len == b_len) return 0;

    /* if a is shorter then b, then b is larger */
    return (a_len - b_len);
}

static const char encoded_chars_rel_uri_part[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
    1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1,  /*  20 -  2F space " # $ % & ' + , / */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  /*  30 -  3F : ; = ? @ < > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  50 -  5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  /*  70 -  7F DEL */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

static const char encoded_chars_rel_uri[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
    1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0,  /*  20 -  2F space " # $ % & ' + , / */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  /*  30 -  3F : ; = ? @ < > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  50 -  5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  /*  70 -  7F DEL */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

static const char encoded_chars_html[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  20 -  2F & */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,  /*  30 -  3F < > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  50 -  5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  /*  70 -  7F DEL */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

static const char encoded_chars_minimal_xml[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
    0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  20 -  2F & */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,  /*  30 -  3F < > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  50 -  5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  /*  70 -  7F DEL */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  80 -  8F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  90 -  9F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  A0 -  AF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  B0 -  BF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  C0 -  CF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  D0 -  DF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  E0 -  EF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  F0 -  FF */
};

static const char encoded_chars_hex[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  20 -  2F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  30 -  3F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  40 -  4F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  50 -  5F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  60 -  6F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  70 -  7F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

static const char encoded_chars_http_header[] = {
    /*
    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,  /*  00 -  0F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  10 -  1F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  20 -  2F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  30 -  3F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  50 -  5F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  70 -  7F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  80 -  8F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  90 -  9F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  A0 -  AF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  B0 -  BF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  C0 -  CF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  D0 -  DF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  E0 -  EF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  F0 -  FF */
};
