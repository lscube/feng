/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include <fenice/utils.h>
//XXX replace
int get_field(uint8_t *d, uint32_t bits, uint32_t *offset)
{
    uint32_t v = 0;
    uint32_t i;

    for( i = 0; i < bits; ) {
        if( bits - i >= 8 && *offset % 8 == 0 ) {
            v <<= 8;
            v |= d[*offset/8];
            i += 8;
            *offset += 8;
        } else {
            v <<= 1;
            v |= ( d[*offset/8] >> ( 7 - *offset % 8 ) ) & 1;
            ++i;
            ++(*offset);
        }
    }
    return v;
}
