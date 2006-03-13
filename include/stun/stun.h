/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#ifndef _STUNH	
#define _STUNH
/*RFC-3489*/

#include <fenice/types.h>

#define STUN_MAX_STRING 256
#define STUN_MAX_MESSAGE_SIZE 2048
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8

/*---- STUN HEADER SECTION ----*/ 

/* Stun Header:
 * _____________________________________________________
 * | Message Type (16 bits) | Message Length (16 bits) |
 * -----------------------------------------------------
 * |						       |
 * |                                                   |
 * |            Transaction ID (128 bits)              |
 * |                                                   |
 * -----------------------------------------------------
 *
 */

struct STUN_HEADER{
	uint16 msgtype;	
	uint32 msglen; /*not including the 20 byte header*/
	uint128 transactionID;
};

/* Define Message Types values */
/*Binding*/
#define BINDING_REQUEST 0x0001 /*B_REQ*/
#define BINDING_RESPONSE 0x0101 /*B_RES*/
#define BINDING_ERROR_RESPONSE 0x0111 /*B_ERR*/
/*Shared Secret (not enabled in the first implementation*/
#define SHARED_SECRET_REQUEST 0x0002 /*S_REQ*/
#define SHARED_SECRET_RESPONSE 0x0102 /*S_RES*/
#define SHARED_SECRET_ERROR_RESPONSE 0x0112 /*S_ERR*/

/*---- END STUN HEADER SECTION ----*/

/*see RFC - page 30 */
#define STUN_BAD_REQUEST_CODE 400
#define STUN_UNAUTHORIZED_CODE 401
#define STUN_UNKNOWN_ATTRIBUTE_CODE 420
#define STUN_STALE_CREDENTIALS_CODE 430
#define STUN_INTEGRITY_CHECK_FAILURE_CODE 431
#define STUN_MISSING_USERNAME_CODE 432
#define STUN_USE_TLS_CODE 433
#define STUN_SERVER_ERROR_CODE 500
#define STUN_GLOBAL_FAILURE_CODE 600



/*---- STUN PAYLOAD SECTION ----*/ 
/*After the header are 0 or more attributes.*/

/* Stun Payload. Each attribute is TLV encoded:
 * _________________________________________________________
 * | Attribute Type (16 bits) | Attribute Length (16 bits) |
 * ---------------------------------------------------------
 * |                                                       |
 * |           Attribute Value (Variable Length)           |
 * |                                                       |
 * ---------------------------------------------------------
 *
 */

/*Define Attribute Types*/		/*B_REQ|B_RES|B_ERR|S_REQ|S_RES|S_ERR*/
/*---------------------------------------------------------------------------*/
#define MAPPED_ADDRESS 0x0001 		/*     |  M  |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define RESPONSE_ADDRESS 0x0002		/*  O  |     |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define CHANGE_REQUEST 0x0003		/*  O  |     |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define SOURCE_ADDRESS 0x0004		/*     |  M  |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define CHANGED_ADDRESS 0x0005		/*     |  M  |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define USERNAME 0x0006			/*  O  |     |     |     |  M  |     */
/*---------------------------------------------------------------------------*/
#define PASSWORD 0x0007			/*     |     |     |     |  M  |     */
/*---------------------------------------------------------------------------*/
#define MESSAGE_INTEGRITY 0x0008	/*  O  |  O  |     |     |     |     */
/*---------------------------------------------------------------------------*/
#define ERROR_CODE 0x0009		/*     |     |  M  |     |     |  M  */
/*---------------------------------------------------------------------------*/
#define UNKNOWN_ATTRIBUTES 0x000A	/*     |     |  C  |     |     |  C  */
/*---------------------------------------------------------------------------*/
#define REFLECTED_FROM 0x000B		/*     |  C  |     |     |     |     */
/*---------------------------------------------------------------------------*/
					/*
					 * M = mandatory
					 * O = optional
					 * C = conditional based on some other 
					 *     aspect of the message
					 *
					 * blank spaces means Not Applicable
					 * */
/*the previous table shows there is at most 4 attributes in a message*/
#define STUN_MAX_MESSAGE_ATRS 4

struct STUN_ATR_HEADER{
      uint16 type;
      uint16 length;
};

/*define structs for different Attribute*/
struct STUN_ATR_ADDRESS { /*MAPPED, RESPONSE, CHANGED, SOURCE, REFLECTED-FROM*/
	uint8 ignored;	 
	uint8 family; /*is always 0x01 = IPv4*/
	uint16 port; /*network byte order*/
	uint32 address;
};

struct STUN_ATR_CHANGE_REQUEST {
	uint32 flagsAB;/*0000000000000000000000000000AB0*/
};		       /* 
			* A = change IP
			* B = change port 
			*/
#define SET_CHANGE_PORT_FLAG(iflag) ( iflag|=0x00000004 )
#define SET_CHANGE_ADDR_FLAG(iflag) ( iflag|=0x00000002 )
#define IS_SET_CHANGE_PORT_FLAG(iflag) ( iflag & 0x00000004 )
#define IS_SET_CHANGE_ADDR_FLAG(iflag) ( iflag & 0x00000002 )

struct STUN_ATR_STRING { /*USERNAME, PASSWORD*/
	uint8 username[STUN_MAX_STRING]; /*Its length MUST be a multiple of 4 bytes*/
};

/*doesn't map the following struct to costruct the pkt. Let cast to the previous one*/
struct STUN_ATR_STRING_CTRL { /*this stuct is usefull only to store the string length*/
	struct STUN_ATR_STRING atr_string;
	uint16 length;
};

struct STUN_ATR_INTEGRITY { /*MUST be the last attribute in any STUN message.*/
	uint8 hash[20]; /* HMAC-SHA1*/
};

struct STUN_ATR_ERROR_CODE {
	uint16 zerobytes; /*0*/
	uint8  error_class;
	uint8 number;
	uint8 reason[STUN_MAX_STRING];
};
/*doesn't map the following struct to costruct the pkt. Let cast to the previous one*/
struct STUN_ATR_ERROR_CODE_CTRL { /*this stuct is usefull only to store the reason length*/
	struct STUN_ATR_ERROR_CODE atr_err_code;
	uint16 length;
};

/*UNKNOWN-ATTRIBUTES is present only in a B_ERR or S_ERR when the response code in
 * the ERROR-CODE is 420*/
/*the following attribute contains a list of 16 bit values each of which
 * represents an attribute type that was not understood by the server. If
 * num_attr is odd, one of the attributes MUST be repeated.*/
struct STUN_ATR_UNKNOWN {
      uint16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
};
/*doesn't map the following struct to costruct the pkt. Let cast to the previous one*/
struct STUN_ATR_UNKNOWN_CTRL { /*this stuct is usefull only to store the number of  */
	struct STUN_ATR_UNKNOWN atr_unknown;				/*  attribute type  */
	uint16 num_attr;;
};
/*---- END STUN PAYLOAD SECTION ----*/

typedef struct STUN_ATR {
	struct STUN_ATR_HEADER stun_atr_hdr;
	void *atr;/*to allocate*/
} stun_atr;

/*Packet*/
typedef struct STUN_PKT {
	struct STUN_HEADER stun_hdr;
	stun_atr *atrs[STUN_MAX_MESSAGE_ATRS];/*to allocate*/
} OMS_STUN_PKT;

/*a mask variable to set end unset the different Attributes*/
typedef struct STUN_PKT_DEV {
	OMS_STUN_PKT stun_pkt;
	uint16 set_atr_mask; 
	uint16 num_message_atrs; /* at most = STUN_MAX_MESSAGE_ATRS*/
	uint8 list_uknown[STUN_MAX_MESSAGE_ATRS];/*0 or 1. List of*/
						/*the unknown attributes*/
						/*in the received*/
						/*message. 0 = UNKNOWN*/
} OMS_STUN_PKT_DEV;

#define IS_VALID_ATR_TYPE(atrtype) ( (atrtype >= MAPPED_ADDRESS) && \
						(atrtype<=REFLECTED_FROM ) )

#define SET_ATRMASK(atrtype,mask) \
	( (IS_VALID_ATR_TYPE(atrtype))?(mask|=( 0x0001 << (atrtype - 1))):0 ) 

/*

  	| Destination | Change IP | Change Port | Return IP:port |
--------+-------------+-----------+-------------+----------------+
Test I	|   IP1:1     |    N      |     N       |      IP1:1     |
--------+-------------+-----------+-------------+----------------+
Test II	|   IP1:1     |    Y      |     Y       |      IP2:2     |
--------+-------------+-----------+-------------+----------------+
Test III|   IP2:1     |    N      |     N       |      IP2:1     |
--------+-------------+-----------+-------------+----------------+
Test IV	|   IP1:1     |    N      |     Y       |      IP1:2     |
--------+-------------+-----------+-------------+----------------+

                        +--------+
                        |  Test  |
                        |   I    |
                        +--------+
                             |
                             |
                             V
                            /\              /\
                         N /  \ Y          /  \ Y             +--------+
          UDP     <-------/Resp\--------->/ IP \------------->|  Test  |
          Blocked         \ ?  /          \Same/              |   II   |
                           \  /            \? /               +--------+
                            \/              \/                    |
                                             | N                  |
                                             |                    V
                                             V                    /\
                                         +--------+  Sym.      N /  \
                                         |  Test  |  UDP    <---/Resp\
                                         |   II   |  Firewall   \ ?  /
                                         +--------+              \  /
                                             |                    \/
                                             V                     |Y
                  /\                         /\                    |
   Symmetric  N  /  \       +--------+   N  /  \                   V
      NAT  <--- / IP \<-----|  Test  |<--- /Resp\               Open
                \Same/      |   I    |     \ ?  /               Internet
                 \? /       +--------+      \  /
                  \/                         \/
                  |                           |Y
                  |                           |
                  |                           V
                  |                           Full
                  |                           Cone
                  V              /\
              +--------+        /  \ Y
              |  Test  |------>/Resp\---->Restricted
              |   III  |       \ ?  /
              +--------+        \  /
                                 \/
                                  |N
                                  |       Port
                                  +------>Restricted

                    Flow for type discovery process
*/

/*API*/
/*
 *parse_stun_message:
 *receives a pkt, parses it and allocates an  OMS_STUN_PKT_DEV by which
 *it is simple to manage message type and attributes.
 *
 *return value: 
 *	one of error code or zero for success
 * */
uint32 parse_stun_message(uint8 *pkt, uint32 pktsize, 
				OMS_STUN_PKT_DEV **pkt_dev_pt);

/*return value: 
 *	one of error code or zero for success
 */
uint32 parse_atrs(OMS_STUN_PKT_DEV *pkt_dev);

uint32 mapped_address(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 response_address(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 change_request(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 source_address(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 changed_address(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 username(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 password(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 message_integrity(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);
uint32 unknown_attribute(OMS_STUN_PKT_DEV *pkt_dev,uint32 idx);

#endif //__STUNH
