/* * 
 *  $Id$
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2001 by
 *      
 *      Giampaolo "mancho" Mancini - manchoz@inwind.it
 *    Francesco "shawill" Varano - shawill@infinto.it
 *
 *  NeMeSI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NeMeSI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NeMeSI; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

/* From RFC 1889 Appendix A.6 */

#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fenice/md5global.h>
#include <fenice/md5.h>
#include <fenice/types.h>

static uint32_t md_32(char *string, int length)
{
    MD5_CTX context;
    union {
        char c[16];
        uint32_t x[4];
    } digest;
    uint32_t r;
    int i;

    MD5Init(&context);
    MD5Update(&context, string, length);
    MD5Final((unsigned char *)&digest, &context);
    r = 0;
    for (i = 0; i < 3; i++)
        r ^= digest.x[i];
    return r;
}

uint32_t random32(int type)
{
    struct {
        int type;
        struct timeval tv;
        clock_t cpu;
        pid_t pid;
        uint32_t hid;
        uid_t uid;
        gid_t gid;
        struct utsname name;
    } s;

    gettimeofday(&s.tv, NULL);
    uname(&s.name);
    s.type = type;
    s.cpu  = clock();
    s.pid  = getpid();
    s.hid  = gethostid();
    s.uid  = getuid();
    s.gid  = getgid();

    return md_32((char *)&s, sizeof(s));
}
