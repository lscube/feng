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

#ifndef __STUN_H	
#define __STUN_H
/*RFC-3489*/

#include <stun/types.h>

#define STUN_MAX_STRING 256
#define STUN_MAX_MESSAGE_SIZE 2048

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
	STUNuint16 msgtype;	
	STUNuint32 msglen; /*not including the 20 byte header*/
	STUNuint128 transactionID;
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
#define STUN_MAX_MESSAGE_ATRS 5 
#define STUN_MAX_UNKNOWN_ATTRIBUTES STUN_MAX_MESSAGE_ATRS 

struct STUN_ATR_HEADER{
      STUNuint16 type;
      STUNuint16 length;
};

typedef struct STUN_ATR {
	struct STUN_ATR_HEADER stun_atr_hdr;
	void *atr;/*to allocate. It can be cast to: */
} stun_atr;	/*
			*	STUN_ATR_ADDRESS
			*	STUN_ATR_CHANGE_REQUEST
			*	STUN_ATR_STRING
			*	STUN_ATR_INTEGRITY
			*	STUN_ATR_ERROR_CODE
			*	STUN_ATR_UNKNOWN
		*/

/*define structs for different Attribute*/
struct STUN_ATR_ADDRESS { /*MAPPED, RESPONSE, CHANGED, SOURCE, REFLECTED-FROM*/
	STUNuint8 ignored;	 
	STUNuint8 family; /*is always 0x01 = IPv4*/
	STUNuint16 port; /*network byte order*/
	STUNuint32 address;
};

struct STUN_ATR_CHANGE_REQUEST {
	STUNuint32 flagsAB;/*0000000000000000000000000000AB0*/
};		       /* A = change IP; B = change port*/
#define SET_CHANGE_PORT_FLAG(iflag) ( iflag|=0x00000004 )
#define SET_CHANGE_ADDR_FLAG(iflag) ( iflag|=0x00000002 )
#define IS_SET_CHANGE_PORT_FLAG(iflag) ( iflag & 0x00000004 )
#define IS_SET_CHANGE_ADDR_FLAG(iflag) ( iflag & 0x00000002 )

struct STUN_ATR_STRING { /*USERNAME, PASSWORD*/
	STUNuint8 username_or_passwd[STUN_MAX_STRING]; /*Its length MUST be a multiple of 4 bytes*/
};

/*doesn't map the following struct to costruct the pkt. Let cast to the previous one*/
struct STUN_ATR_STRING_CTRL { /*this stuct is usefull only to store the string length*/
	struct STUN_ATR_STRING atr_string;
	STUNuint16 length;
};

struct STUN_ATR_INTEGRITY { /*MUST be the last attribute in any STUN message.*/
	STUNuint8 hash[20]; /* HMAC-SHA1*/
};

#define STUN_MAX_ERROR_REASON 256
struct STUN_ATR_ERROR_CODE {
	STUNuint16 zerobytes; /*0*/
	STUNuint16  error_class_number; /* [1..6] */
	STUNuint8 reason[STUN_MAX_ERROR_REASON]; /*STUN_ATR_CODE has fixed size. The unused space
					in reason is filled with \0 char*/
};
/*see RFC - page 30 */
struct STUN_ATR_ERROR_CODE ERROR_CODE_MTRX[] = {
	{0, 0, ""},
	{0, 400, "Bad Request"},
	{0, 401, "Unauthorized"},
	{0, 420, "Unknown Attribute"},
	{0, 430, "Stale Credentials"},
	{0, 431, "Integrity Check Failure"},
	{0, 432, "Missing Username"},
	{0, 433, "Use TLS"},
	{0, 500, "Server Error"},
	{0, 600, "Global Failure"}
};
#define STUN_BAD_REQUEST		1	
#define STUN_UNAUTHORIZED		2
#define STUN_UNKNOWN_ATTRIBUTE 		3
#define STUN_STALE_CREDENTIALS 		4
#define STUN_INTEGRITY_CHECK_FAILURE 	5
#define STUN_MISSING_USERNAME 		6	
#define STUN_USE_TLS			7
#define STUN_SERVER_ERROR		8
#define STUN_GLOBAL_FAILURE 		9

/*UNKNOWN-ATTRIBUTES is present only in a B_ERR or S_ERR when the response code in
 * the ERROR-CODE is 420*/
/*the following attribute contains a list of 16 bit values each of which
 * represents an attribute type that was not understood by the server. If
 * num_attr is odd, one of the attributes MUST be repeated.*/
struct STUN_ATR_UNKNOWN {
	STUNuint16 *attrType;
};

/*---- END STUN PAYLOAD SECTION ----*/

/*General Stun Packet*/
typedef struct STUN_PKT {
	struct STUN_HEADER stun_hdr;
	stun_atr *atrs[STUN_MAX_MESSAGE_ATRS];/*to allocate*/
} OMS_STUN_PKT;

/*a mask variable to set end unset the different Attributes*/
/*this struct is used to parse the received pkt*/
typedef struct STUN_PKT_DEV {
	OMS_STUN_PKT stun_pkt; /*received*/

	STUNuint8 num_message_atrs; /* at most = STUN_MAX_MESSAGE_ATRS*/
	STUNuint8 num_unknown_atrs; /*the num of attribute type*/
	STUNuint16 list_unknown_attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];

} OMS_STUN_PKT_DEV;

#define IS_VALID_ATR_TYPE(atrtype) ( (atrtype >= MAPPED_ADDRESS) && \
						(atrtype<=REFLECTED_FROM ) )

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
									  
/* --- RECEIVER SIDE --- */
/*
 *parse_stun_message:
 *receives a pkt, parses it and allocates an  OMS_STUN_PKT_DEV by which
 *it is simple to manage message type and attributes.
 *
 *return value: 
 *	one of error code or zero for success
 * */
STUNuint32 parse_stun_message(STUNuint8 *pkt, STUNuint32 pktsize,
		OMS_STUN_PKT_DEV **pkt_dev_pt);
void free_pkt_dev(OMS_STUN_PKT_DEV *pkt_dev);


/*PARSER FUNCTION (received pkts) for the attributes*/
/*return value: 
 *	one of error code or zero for success
 */
STUNuint32 parse_atrs(OMS_STUN_PKT_DEV *pkt_dev);

STUNuint32 parse_mapped_address(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_response_address(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_change_request(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_source_address(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_changed_address(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_unknown_attribute(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_reflected_from(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_error_code(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
/*TODO: */
STUNuint32 parse_username(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_password(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);
STUNuint32 parse_message_integrity(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx);


/* --- SENDER SIDE --- */
STUNint32 stun_rand();
STUNuint128 create_transactionID(STUNuint32 testNum);

/*CREATE ATRIBUTES (sent pktS)*/
void add_stun_atr_hdr(stun_atr *atr, STUNuint16 type, STUNuint16 len);
/*returned value:
 *	stun_atr* on success, otherside NULL
 */
/*common function for MAPPED, RESPONSE, CHANGED, SOURCE, REFLECTED-FROM */
stun_atr *create_address(STUNuint8 family, STUNuint16 port, STUNuint32 address, STUNuint16 type);
inline stun_atr *create_mapped_address(STUNuint8 family, STUNuint16 port, STUNuint32 address);
inline stun_atr *create_response_address(STUNuint8 family, STUNuint16 port, STUNuint32 address);
inline stun_atr *create_source_address(STUNuint8 family, STUNuint16 port, STUNuint32 address);
inline stun_atr *create_changed_address(STUNuint8 family, STUNuint16 port, STUNuint32 address);
inline stun_atr *create_reflected_from(STUNuint8 family, STUNuint16 port, STUNuint32 address);

stun_atr *create_change_request(STUNbool port, STUNbool addr);
stun_atr *create_unknown_attribute(OMS_STUN_PKT_DEV *received);
stun_atr *create_error_code(STUNuint8 error_code_type); 
/*
error_code_type can be:

	STUN_BAD_REQUEST		
	STUN_UNAUTHORIZED		
	STUN_UNKNOWN_ATTRIBUTE 		
	STUN_STALE_CREDENTIALS 		
	STUN_INTEGRITY_CHECK_FAILURE 	
	STUN_MISSING_USERNAME 		
	STUN_USE_TLS			
	STUN_SERVER_ERROR		
	STUN_GLOBAL_FAILURE
*/

/*TODO: */
stun_atr *create_username();
stun_atr *create_password();
stun_atr *create_message_integrity();

#endif //__STUNH
