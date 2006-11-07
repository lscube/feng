
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> /*ntohl*/
#include <stun/stun.h>
#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>
//#include <glib.h>
//#include <glib/gprintf.h>

static uint32 __sa(STUNbool caddr, STUNbool cport, uint32 idx_socks, stun_atr **atr, OMSStunServer *omss)
{
	uint32 sa;
	
	if( cport && caddr ) {
		sa = htonl( get_local_s_addr( ((omss->sock[idx_socks]).change_port_addr)) );
		//source address
		*atr = create_source_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_port_addr), sa);
	}
	else if( cport ) {
		sa = htonl(  get_local_s_addr( ((omss->sock[idx_socks]).change_port)) );
		//source address
		*atr = create_source_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_port), sa);
	}
	else if ( caddr ) {
		sa = htonl( get_local_s_addr( ((omss->sock[idx_socks]).change_addr)) );
		//source address
		*atr = create_source_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_addr), sa);
	}
	else {
		sa = htonl( get_local_s_addr( ((omss->sock[idx_socks]).sock)) ); 
		//source address
		*atr = create_source_address(IPv4family, get_local_port((omss->sock[idx_socks]).sock), sa);
	}
	return sa;

}

static int32 send_binding_reponse(STUNbool caddr, STUNbool cport, uint32 idx_socks,
	OMSStunServer *omss, void *pkt, uint32 pkt_size, struct sockaddr_storage stg)
{
	int32 n = 0;

	if( cport && caddr ) {
		n = Sock_write( (omss->sock[idx_socks]).change_port_addr, pkt, pkt_size, &stg); 
	}
	else if( cport ) {
		n = Sock_write ( (omss->sock[idx_socks]).change_port, pkt, pkt_size, &stg);
	}
	else if ( caddr ) {
		n = Sock_write( (omss->sock[idx_socks]).change_addr, pkt, pkt_size, &stg);
	}
	else {
		n = Sock_write( (omss->sock[idx_socks]).sock, pkt, pkt_size, &stg);
	}

#if 0
	printf("remote port: %d - remote addr: %s\n",atoi(get_remote_port((omss->sock[idx_socks]).sock)), \
		      get_remote_host((omss->sock[idx_socks]).sock));
	printf("local port: %d - local addr: %s\n",atoi(get_local_port((omss->sock[idx_socks]).sock)), \
		      get_local_host((omss->sock[idx_socks]).sock));
#endif
	return n;

}

void binding_response(OMS_STUN_PKT_DEV *pkt_dev, OMSStunServer *omss, uint32 idx_socks, struct sockaddr_storage stg)
{
	uint32 ma = ((struct sockaddr_in *)&stg)->sin_addr.s_addr;
	uint32 sa, ca;
	uint16 atrlen = 0;
	uint32 wbytes = 0;
	uint16 msglen = 0;
	uint32 idx_ca = pkt_dev->idx_atr_type_list[CHANGE_REQUEST];
	int32 idx_ra = pkt_dev->idx_atr_type_list[RESPONSE_ADDRESS];
	STUNbool cport = STUN_FALSE;
	STUNbool caddr = STUN_FALSE;
	struct STUN_HEADER *stun_hdr = calloc(1,sizeof(struct STUN_HEADER));
	stun_atr *atr;
	uint32 n;
	uint8 pkt[STUN_MAX_MESSAGE_SIZE];
	
	/*if change port is present*/
	//if((struct STUN_ATR_CHANGE_REQUEST *)((pkt_dev->stun_pkt).atrs[idx_ca]) != NULL)
	if(pkt_dev->idx_atr_type_list[CHANGE_REQUEST])
	{
		cport = IS_SET_CHANGE_PORT_FLAG( ntohl(( (struct STUN_ATR_CHANGE_REQUEST *)((pkt_dev->stun_pkt).atrs[idx_ca])->atr)->flagsAB ));
		caddr = IS_SET_CHANGE_ADDR_FLAG( ntohl(( (struct STUN_ATR_CHANGE_REQUEST *)((pkt_dev->stun_pkt).atrs[idx_ca])->atr)->flagsAB ));
 	}
	

	memset(pkt,0,sizeof(pkt));

	stun_hdr->msgtype = BINDING_RESPONSE;

	memcpy(&(stun_hdr->transactionID), &((pkt_dev->stun_pkt).stun_hdr).transactionID, sizeof(STUNuint128));
	
	wbytes = sizeof(struct STUN_HEADER);
		
	//mapped address
	atr = create_mapped_address(IPv4family, ((struct sockaddr_in*)&stg)->sin_port, ma);
	
	atrlen = sizeof(struct STUN_ATR_ADDRESS) + SIZE_ATR_HDR;
	
	memcpy(&(pkt[wbytes]), atr, SIZE_ATR_HDR); 
	wbytes += SIZE_ATR_HDR;
	msglen += SIZE_ATR_HDR;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->ignored;
	wbytes++;
	msglen++;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->family;
	wbytes++;
	msglen++;
	
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->port), 2);
	wbytes += 2;
	msglen += 2;
	
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->address), 4);
	wbytes += 4;
	msglen += 4;
	free(atr);

	//source address
	sa = __sa(caddr, cport, idx_socks, &atr, omss);
	atrlen = sizeof(struct STUN_ATR_ADDRESS) + SIZE_ATR_HDR;
	memcpy(&(pkt[wbytes]), atr, SIZE_ATR_HDR); 
	wbytes += SIZE_ATR_HDR;
	msglen += SIZE_ATR_HDR;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->ignored;
	wbytes++;
	msglen++;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->family;
	wbytes++;
	msglen++;
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->port), 2);
	wbytes += 2;
	msglen += 2;
	
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->address), 4);
	wbytes += 4;
	msglen += 4;
	free(atr);


	//changed address
	if( cport && caddr ) {
		ca = htonl( get_local_s_addr( (omss->sock[idx_socks]).change_port_addr ));
		atr = create_changed_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_port_addr), ca);
	}
	else if( cport ) {
		ca = htonl( get_local_s_addr( (omss->sock[idx_socks]).change_port ));
		atr = create_changed_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_port), ca);
	}
	else if ( caddr ) {
		ca = htonl( get_local_s_addr( (omss->sock[idx_socks]).change_addr ));
		atr = create_changed_address(IPv4family, get_local_port((omss->sock[idx_socks]).change_addr), ca);
	}
	else {
		ca = htonl( get_local_s_addr( (omss->sock[idx_socks]).sock ));
		atr = create_changed_address(IPv4family, get_local_port((omss->sock[idx_socks]).sock), ca);
	}

	//printf("flagAB = %d\n",ntohl(((struct STUN_ATR_CHANGE_REQUEST *)(((pkt_dev->stun_pkt).atrs[idx_ca])->atr))->flagsAB));
	
	atrlen = sizeof(struct STUN_ATR_ADDRESS) + SIZE_ATR_HDR;
	
	memcpy(&(pkt[wbytes]), atr, SIZE_ATR_HDR); 
	wbytes += SIZE_ATR_HDR;
	msglen += SIZE_ATR_HDR;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->ignored;
	wbytes++;
	msglen++;
	pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->family;
	wbytes++;
	msglen++;
	
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->port), 2);
	wbytes += 2;
	msglen += 2;
	
	memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->address), 4);
	wbytes += 4;
	msglen += 4;
	free(atr);



#if 0
	//reflected from
	if(idx_ra != -1) { //if response addr present. See parse_atrs
	
		atr = create_reflected_from(IPv4family, 0, 0); //only to allocate atr
		atrlen = sizeof(struct STUN_ATR_ADDRESS) + SIZE_ATR_HDR;

		memcpy(atr,(pkt_dev->stun_pkt).atrs[idx_ra], atrlen) ;
		atr->stun_atr_hdr.type = REFLECTED_FROM;
		
		memcpy(&(pkt[wbytes]), atr, SIZE_ATR_HDR); 
		wbytes += SIZE_ATR_HDR;
		msglen += SIZE_ATR_HDR;
		pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->ignored;
		wbytes++;
		msglen++;
		pkt[wbytes] = ((struct STUN_ATR_ADDRESS *)(atr->atr))->family;
		wbytes++;
		msglen++;
		
		memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->port), 2);
		wbytes += 2;
		msglen += 2;
	
		memcpy(&(pkt[wbytes]), &(((struct STUN_ATR_ADDRESS *)(atr->atr))->address), 4);
		wbytes += 4;
		msglen += 4;
		free(atr);
	}
#endif //0

	
	stun_hdr->msglen = htons(msglen);
	memcpy(pkt,stun_hdr,sizeof(struct STUN_HEADER));

	send_binding_reponse(caddr, cport, idx_socks, omss, pkt, wbytes, stg);


}

