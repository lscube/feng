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

#include <stdio.h>
#include <string.h>

#include <fenice/utils.h>

int get_utc(struct tm *t, char *b)
{
    char tmp[5];
    if (strlen(b)<16) {
        return ERR_GENERIC;
    }
    // Date
    strncpy(tmp,b,4);
    tmp[4]='\0';
    sscanf(tmp,"%d",&(t->tm_year));
    strncpy(tmp,b+4,2);
    tmp[2]='\0';
    sscanf(tmp,"%d",&(t->tm_mon));
    strncpy(tmp,b+6,2);
    tmp[2]='\0';
    sscanf(tmp,"%d",&(t->tm_mday));    
    // Time
    strncpy(tmp,b+9,2);
    tmp[2]='\0';
    sscanf(tmp,"%d",&(t->tm_hour));    
    strncpy(tmp,b+11,2);
    tmp[2]='\0';
    sscanf(tmp,"%d",&(t->tm_min));    
    strncpy(tmp,b+13,2);
    tmp[2]='\0';
    sscanf(tmp,"%d",&(t->tm_sec));        
    return ERR_NOERROR;
}

