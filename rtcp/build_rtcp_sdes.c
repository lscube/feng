/* * 
 *  $Id: build_rtcp_sdes.c 53 2004-01-16 17:45:52Z shawill $
 *  
 *  This file is part of NeMeSI
 *
 *  NeMeSI -- NEtwork MEdia Streamer I
 *
 *  Copyright (C) 2001 by
 *  	
 *  	Giampaolo "mancho" Mancini - manchoz@inwind.it
 *	Francesco "shawill" Varano - shawill@infinto.it
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

#include <fenice/rtcp.h>
#include <fenice/socket.h>
//#include <fenice/version.h>
#include <pwd.h>

int build_rtcp_sdes(RTP_session *session, unsigned char *pkt, int left)
{
	struct passwd *pwitem=getpwuid(getuid());
	rtcp_sdes_item_t *item;
	char str[MAX_SDES_LEN];
	int len, pad;

	memset(str, 0, MAX_SDES_LEN);
	
	/* SDES CNAME: username@ipaddress */
	strcpy(str, pwitem->pw_name);
	strcat(str, "@");
	strcat(str, get_address());
	if ( ((strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t) +1) >> 2)  > (unsigned int)left )
		/* No space left in UDP pkt */
		return 0;
	
	len = (strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t)) >> 2;
	pkt->common.ver=RTP_VERSION;
	pkt->common.pad=0;
	pkt->common.count=1;
	pkt->common.pt=RTCP_SDES;
	pkt->r.sdes.src=htonl(session->ssrc);

	item=pkt->r.sdes.item;

	item->type=RTCP_SDES_CNAME;
	item->len=strlen(str);
	strcpy((char *)item->data, str);
	
	item = (rtcp_sdes_item_t *)((char *)item + strlen((char *)item));

	/* SDES NAME: real name, if it exists */
	if (strlen(strcpy(str, pwitem->pw_gecos))) {
		if ( ((strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t) + 1) >> 2) > (unsigned int)left ){
			/* No space left in UDP pkt */
			pad = 4 - len % 4;
			len += pad/4;
			while (pad--)
				*(((char *)item)++)=0;
			pkt->common.len=htons(len);
			return len;
		}
	
		len += (strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t) + 1) >> 2;

		item->type=RTCP_SDES_NAME;
		item->len=strlen(str);
		strcpy((char *)item->data, str);
	
		item = (rtcp_sdes_item_t *)((char *)item + strlen((char *)item));
	}
	
	/* SDES TOOL */
	sprintf(str, "%s - %s", PROG_NAME, PROG_DESCR);
	if ( ((strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t)) >> 2) > (unsigned int)left ){
		/* No space left in UDP pkt */
		pad = 4 - len % 4;
		len += pad/4;
		while (pad--)
			*(((char *)item)++)=0;
		pkt->common.len=htons(len);
		return len;
	}
	
	len += (strlen(str) + sizeof(rtcp_sdes_item_t) - 1 + sizeof(rtcp_common_t) + 1) >> 2;

	item->type=RTCP_SDES_TOOL;
	item->len=strlen(str);
	strcpy((char *)item->data, str);

	item = (rtcp_sdes_item_t *)((char *)item + strlen((char *)item));

	pad = 4 - len % 4;
	len += pad/4;
	while (pad--)
		*(((char *)item)++)=0;
	pkt->common.len=htons(len);

	return len;
}
