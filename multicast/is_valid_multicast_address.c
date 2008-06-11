/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
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

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fenice/safe-ctype.h> /*ISXDIGIT*/

#include <fenice/multicast.h>
#include <fenice/utils.h>

#ifndef IN_IS_ADDR_MULTICAST
#define IN_IS_ADDR_MULTICAST(a)    ((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((__const uint8_t *) (a))[0] == 0xff)
#endif


/* The following two functions were adapted from glibc's
   implementation of inet_pton, written by Paul Vixie. */

static bool is_valid_ipv4_address (const char *str, const char *end)
{
    bool saw_digit = false;
    int octets = 0;
    int val = 0;

    while (str < end) {
        int ch = *str++;

        if (ch >= '0' && ch <= '9') {
            val = val * 10 + (ch - '0');
            if (val > 255)
                return false;
            if (!saw_digit) {
                if (++octets > 4)
                    return false;
                saw_digit = true;
            }
        }
        else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return false;
            val = 0;
            saw_digit = false;
        }
        else
            return false;
    }
    if (octets < 4)
        return false;
  
    return true;
}

static bool is_valid_ipv6_address (const char *str, const char *end)
{
    /* Use lower-case for these to avoid clash with system headers.  */
    enum {
        ns_inaddrsz  = 4,
        ns_in6addrsz = 16,
        ns_int16sz   = 2
    };

    const char *curtok;
    int tp;
    const char *colonp;
    bool saw_xdigit;
    unsigned int val;

    tp = 0;
    colonp = NULL;

    if (str == end)
        return false;
  
    /* Leading :: requires some special handling. */
    if (*str == ':') {
        ++str;
        if (str == end || *str != ':')
            return false;
    }

    curtok = str;
    saw_xdigit = false;
    val = 0;

    while (str < end) {
        int ch = *str++;

        /* if ch is a number, add it to val. */
        if (ISXDIGIT (ch)) {
            val <<= 4;
            val |= XDIGIT_TO_NUM (ch);
            if (val > 0xffff)
                return false;
            saw_xdigit = true;
            continue;
        }

        /* if ch is a colon ... */
        if (ch == ':') {
            curtok = str;
            if (!saw_xdigit) {
                if (colonp != NULL)
                    return false;
                colonp = str + tp;
                continue;
            }
            else if (str == end)
                return false;
            if (tp > ns_in6addrsz - ns_int16sz)
                return false;
            tp += ns_int16sz;
            saw_xdigit = false;
            val = 0;
            continue;
        }
        /* if ch is a dot ... */
        if (ch == '.' && (tp <= ns_in6addrsz - ns_inaddrsz)
                && is_valid_ipv4_address (curtok, end) == 1) {
            tp += ns_inaddrsz;
            saw_xdigit = false;
            break;
        }
    
        return false;
    }
    if (saw_xdigit) {
        if (tp > ns_in6addrsz - ns_int16sz) 
            return false;
        tp += ns_int16sz;
    }

    if (colonp != NULL) {
        if (tp == ns_in6addrsz) 
            return false;
        tp = ns_in6addrsz;
    }

    if (tp != ns_in6addrsz)
        return false;


    return true;
}


int is_valid_multicast_address(char *ip)
{
    sa_family_t family;

    if(!ip)
        return ERR_PARSE;

    if(is_valid_ipv4_address (ip, &ip[strlen(ip)-1]))
        family = AF_INET;
    else if(is_valid_ipv6_address (ip, &ip[strlen(ip)-1]))
        family = AF_INET6;
    else    
        return ERR_PARSE;
    
    switch (family) {
        case AF_INET: {
            struct in_addr haddr;
            if(!inet_aton(ip, &haddr))
                return ERR_PARSE;  /* not a valid address */

            if (IN_IS_ADDR_MULTICAST(htonl( haddr.s_addr )))
                return ERR_NOERROR;

        }
#ifdef  IPV6
        case AF_INET6: {
            if (IN6_IS_ADDR_MULTICAST(ip))
                return ERR_NOERROR;
        }
#endif
#ifdef  AF_UNIX
        case AF_UNIX:
            return ERR_GENERIC;
#endif
#ifdef  HAVE_SOCKADDR_DL_STRUCT
        case AF_LINK: 
            return ERR_GENERIC;
#endif
        default:
            return ERR_GENERIC;
    }
    
    return ERR_GENERIC;
}

