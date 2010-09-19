/* This file is part of feng
 *
 * Copyright (C) 2010 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <stdbool.h>
#include "feng.h"

/**
 * @brief Ensures that a string is made up of only unreserved characters
 *
 * @param string NULL-terminated string to verify the validity of
 *
 * @retval true All the characters on the string are "unreserved"
 * @retval false At least one character in the string is not "unreserved"
 */
gboolean feng_str_is_unreserved(const char *string)
{
    /* Convert to unsigned integers so it's easier to compare them. */
    uint8_t *data = (uint8_t*)string;

    static const gboolean unreserved_char[128] = {
        ['0'] = true,
        ['1'] = true,
        ['2'] = true,
        ['3'] = true,
        ['4'] = true,
        ['5'] = true,
        ['6'] = true,
        ['7'] = true,
        ['8'] = true,
        ['9'] = true,
        ['a'] = true,
        ['b'] = true,
        ['c'] = true,
        ['d'] = true,
        ['e'] = true,
        ['f'] = true,
        ['g'] = true,
        ['h'] = true,
        ['i'] = true,
        ['j'] = true,
        ['k'] = true,
        ['l'] = true,
        ['m'] = true,
        ['n'] = true,
        ['o'] = true,
        ['p'] = true,
        ['q'] = true,
        ['r'] = true,
        ['s'] = true,
        ['t'] = true,
        ['u'] = true,
        ['v'] = true,
        ['w'] = true,
        ['x'] = true,
        ['y'] = true,
        ['z'] = true,
        ['A'] = true,
        ['B'] = true,
        ['C'] = true,
        ['D'] = true,
        ['E'] = true,
        ['F'] = true,
        ['G'] = true,
        ['H'] = true,
        ['I'] = true,
        ['J'] = true,
        ['K'] = true,
        ['L'] = true,
        ['M'] = true,
        ['N'] = true,
        ['O'] = true,
        ['P'] = true,
        ['Q'] = true,
        ['R'] = true,
        ['S'] = true,
        ['T'] = true,
        ['U'] = true,
        ['V'] = true,
        ['W'] = true,
        ['X'] = true,
        ['Y'] = true,
        ['Z'] = true,
        ['-'] = true,
        ['.'] = true,
        ['_'] = true,
        ['~'] = true
    };

    if ( string == NULL )
        return false;

    while ( *data != '\0' ) {
        if ( *data > 127 ||
             !unreserved_char[*data] )
            return false;
        data++;
    }

    return true;
}
